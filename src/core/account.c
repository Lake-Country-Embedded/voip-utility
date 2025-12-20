/*
 * voip-utility - SIP VoIP Testing Utility
 * SIP Account management implementation
 */

#include "core/account.h"
#include "core/sip_ua.h"
#include "util/log.h"
#include "util/error.h"
#include "util/time_util.h"
#include <string.h>
#include <stdio.h>

const char *vu_account_state_name(vu_account_state_t state)
{
    switch (state) {
    case VU_ACCOUNT_STATE_UNREGISTERED:        return "unregistered";
    case VU_ACCOUNT_STATE_REGISTERING:         return "registering";
    case VU_ACCOUNT_STATE_REGISTERED:          return "registered";
    case VU_ACCOUNT_STATE_REGISTRATION_FAILED: return "failed";
    case VU_ACCOUNT_STATE_UNREGISTERING:       return "unregistering";
    default:                                    return "unknown";
    }
}

vu_error_t vu_account_manager_init(vu_account_manager_t *mgr, const vu_config_t *config)
{
    if (!mgr) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "mgr is NULL");
        return VU_ERR_INVALID_ARG;
    }

    memset(mgr, 0, sizeof(*mgr));

    /* Initialize accounts with -1 pjsua_id */
    for (int i = 0; i < VU_MAX_ACCOUNTS; i++) {
        mgr->accounts[i].pjsua_id = PJSUA_INVALID_ID;
    }

    /* Add accounts from config */
    if (config) {
        for (int i = 0; i < config->account_count; i++) {
            vu_error_t err = vu_account_add(mgr, &config->accounts[i]);
            if (err != VU_OK) {
                VU_LOG_WARN("Failed to add account %s: %s",
                           config->accounts[i].id, vu_error_str(err));
            }
        }
    }

    return VU_OK;
}

void vu_account_manager_cleanup(vu_account_manager_t *mgr)
{
    if (!mgr) return;

    vu_account_unregister_all(mgr);
    memset(mgr, 0, sizeof(*mgr));
}

vu_error_t vu_account_add(vu_account_manager_t *mgr, const vu_account_config_t *config)
{
    if (!mgr || !config) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "mgr or config is NULL");
        return VU_ERR_INVALID_ARG;
    }

    if (vu_account_find(mgr, config->id)) {
        VU_SET_ERROR(VU_ERR_ALREADY_EXISTS, "Account '%s' already exists", config->id);
        return VU_ERR_ALREADY_EXISTS;
    }

    if (mgr->account_count >= VU_MAX_ACCOUNTS) {
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Max accounts (%d) reached", VU_MAX_ACCOUNTS);
        return VU_ERR_NO_MEMORY;
    }

    vu_account_t *acc = &mgr->accounts[mgr->account_count];
    acc->config = *config;
    acc->pjsua_id = PJSUA_INVALID_ID;
    acc->state = VU_ACCOUNT_STATE_UNREGISTERED;

    mgr->account_count++;

    VU_LOG_DEBUG("Added account: %s", config->id);
    return VU_OK;
}

vu_error_t vu_account_remove(vu_account_manager_t *mgr, const char *account_id)
{
    if (!mgr || !account_id) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "mgr or account_id is NULL");
        return VU_ERR_INVALID_ARG;
    }

    for (int i = 0; i < mgr->account_count; i++) {
        if (strcmp(mgr->accounts[i].config.id, account_id) == 0) {
            /* Unregister if registered */
            vu_account_unregister(&mgr->accounts[i]);

            /* Shift remaining accounts */
            for (int j = i; j < mgr->account_count - 1; j++) {
                mgr->accounts[j] = mgr->accounts[j + 1];
            }
            mgr->account_count--;
            return VU_OK;
        }
    }

    VU_SET_ERROR(VU_ERR_NOT_FOUND, "Account '%s' not found", account_id);
    return VU_ERR_NOT_FOUND;
}

vu_account_t *vu_account_find(vu_account_manager_t *mgr, const char *account_id)
{
    if (!mgr || !account_id) return NULL;

    for (int i = 0; i < mgr->account_count; i++) {
        if (strcmp(mgr->accounts[i].config.id, account_id) == 0) {
            return &mgr->accounts[i];
        }
    }
    return NULL;
}

vu_account_t *vu_account_find_by_pjsua_id(vu_account_manager_t *mgr, pjsua_acc_id pjsua_id)
{
    if (!mgr || pjsua_id == PJSUA_INVALID_ID) return NULL;

    for (int i = 0; i < mgr->account_count; i++) {
        if (mgr->accounts[i].pjsua_id == pjsua_id) {
            return &mgr->accounts[i];
        }
    }
    return NULL;
}

vu_error_t vu_account_register(vu_account_t *account)
{
    if (!account) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "account is NULL");
        return VU_ERR_INVALID_ARG;
    }

    if (!vu_ua_is_running()) {
        VU_SET_ERROR(VU_ERR_NOT_INITIALIZED, "UA not initialized");
        return VU_ERR_NOT_INITIALIZED;
    }

    /* Build SIP URI */
    char id_uri[512];
    char registrar_uri[512];
    const vu_account_config_t *cfg = &account->config;

    if (cfg->display_name[0]) {
        snprintf(id_uri, sizeof(id_uri), "\"%s\" <sip:%s@%s>",
                 cfg->display_name, cfg->username, cfg->server);
    } else {
        snprintf(id_uri, sizeof(id_uri), "sip:%s@%s",
                 cfg->username, cfg->server);
    }

    snprintf(registrar_uri, sizeof(registrar_uri), "sip:%s:%d",
             cfg->server, cfg->port);

    /* Configure PJSUA account */
    pjsua_acc_config acc_cfg;
    pjsua_acc_config_default(&acc_cfg);

    acc_cfg.id = pj_str(id_uri);
    acc_cfg.reg_uri = pj_str(registrar_uri);
    acc_cfg.reg_timeout = cfg->reg_timeout_sec;
    acc_cfg.reg_retry_interval = cfg->reg_retry_interval_sec;

    /* Configure RTP port - use 0 to let PJSUA pick an available port */
    pjsua_transport_config_default(&acc_cfg.rtp_cfg);
    acc_cfg.rtp_cfg.port = 0;  /* Auto-select RTP port */

    /* Add credentials */
    acc_cfg.cred_count = 1;
    /* Use "*" to match any realm if no specific realm is configured */
    acc_cfg.cred_info[0].realm = pj_str(cfg->realm[0] ? (char*)cfg->realm : "*");
    acc_cfg.cred_info[0].scheme = pj_str("digest");
    acc_cfg.cred_info[0].username = pj_str((char*)cfg->username);
    acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
    acc_cfg.cred_info[0].data = pj_str((char*)cfg->password);

    /* Set transport */
    switch (cfg->transport) {
    case VU_TRANSPORT_TCP:
        /* Would need to add TCP transport */
        break;
    case VU_TRANSPORT_TLS:
        /* Would need to add TLS transport */
        break;
    default:
        break;
    }

    /* Add account to PJSUA */
    pj_status_t status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &account->pjsua_id);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_SIP_ACCOUNT, status,
                          "Failed to add account %s", cfg->id);
        return VU_ERR_SIP_ACCOUNT;
    }

    account->state = VU_ACCOUNT_STATE_REGISTERING;
    VU_LOG_INFO("Registering account: %s -> %s", cfg->id, registrar_uri);

    return VU_OK;
}

vu_error_t vu_account_unregister(vu_account_t *account)
{
    if (!account) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "account is NULL");
        return VU_ERR_INVALID_ARG;
    }

    if (account->pjsua_id == PJSUA_INVALID_ID) {
        return VU_OK;  /* Not registered */
    }

    account->state = VU_ACCOUNT_STATE_UNREGISTERING;

    /* Set registration to unregister */
    pjsua_acc_set_registration(account->pjsua_id, PJ_FALSE);

    /* Delete account from PJSUA */
    pjsua_acc_del(account->pjsua_id);
    account->pjsua_id = PJSUA_INVALID_ID;
    account->state = VU_ACCOUNT_STATE_UNREGISTERED;

    VU_LOG_INFO("Unregistered account: %s", account->config.id);
    return VU_OK;
}

vu_error_t vu_account_register_all(vu_account_manager_t *mgr)
{
    if (!mgr) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "mgr is NULL");
        return VU_ERR_INVALID_ARG;
    }

    vu_error_t result = VU_OK;

    for (int i = 0; i < mgr->account_count; i++) {
        if (mgr->accounts[i].config.enabled) {
            vu_error_t err = vu_account_register(&mgr->accounts[i]);
            if (err != VU_OK) {
                VU_LOG_WARN("Failed to register account %s: %s",
                           mgr->accounts[i].config.id, vu_error_str(err));
                result = err;
            }
        }
    }

    return result;
}

void vu_account_unregister_all(vu_account_manager_t *mgr)
{
    if (!mgr) return;

    for (int i = 0; i < mgr->account_count; i++) {
        vu_account_unregister(&mgr->accounts[i]);
    }
}

vu_error_t vu_account_wait_registration(vu_account_t *account, int timeout_sec)
{
    if (!account) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "account is NULL");
        return VU_ERR_INVALID_ARG;
    }

    vu_timer_t timer;
    vu_timer_start(&timer, timeout_sec * 1000);

    while (!vu_timer_expired(&timer)) {
        if (account->state == VU_ACCOUNT_STATE_REGISTERED) {
            return VU_OK;
        }
        if (account->state == VU_ACCOUNT_STATE_REGISTRATION_FAILED) {
            VU_SET_ERROR(VU_ERR_REGISTRATION_FAILED,
                        "Registration failed: %s", account->last_status_text);
            return VU_ERR_REGISTRATION_FAILED;
        }

        vu_ua_poll(100);
    }

    VU_SET_ERROR(VU_ERR_TIMEOUT, "Registration timeout for %s", account->config.id);
    return VU_ERR_TIMEOUT;
}

void vu_account_on_reg_state(vu_account_t *account, int code, const char *reason)
{
    if (!account) return;

    account->last_status_code = code;
    if (reason) {
        strncpy(account->last_status_text, reason, sizeof(account->last_status_text) - 1);
    }

    if (code >= 200 && code < 300) {
        account->state = VU_ACCOUNT_STATE_REGISTERED;
        account->registration_time_ms = vu_time_now_ms();
        VU_LOG_INFO("Account %s registered successfully", account->config.id);
    } else if (code >= 400) {
        account->state = VU_ACCOUNT_STATE_REGISTRATION_FAILED;
        VU_LOG_ERROR("Account %s registration failed: %d %s",
                     account->config.id, code, reason ? reason : "");
    }
}
