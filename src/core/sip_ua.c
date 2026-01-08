/*
 * voip-utility - SIP VoIP Testing Utility
 * SIP User Agent implementation
 */

#include "core/sip_ua.h"
#include "core/account.h"
#include "core/call.h"
#include "util/log.h"
#include "util/error.h"
#include <string.h>

/* Global UA state */
static struct {
    vu_ua_state_t state;
    vu_ua_callbacks_t callbacks;
    vu_account_manager_t *acc_mgr;
    vu_call_manager_t *call_mgr;
    pj_caching_pool cp;
    pj_pool_t *pool;
    pjsua_transport_id udp_transport_id;
    bool initialized;
} g_ua = {0};

/* Forward declarations for PJSUA callbacks */
static void on_reg_state(pjsua_acc_id acc_id);
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
                             pjsip_rx_data *rdata);
static void on_call_state(pjsua_call_id call_id, pjsip_event *e);
static void on_call_media_state(pjsua_call_id call_id);
static void on_dtmf_digit2(pjsua_call_id call_id, const pjsua_dtmf_info *info);

vu_ua_config_t vu_ua_default_config(void)
{
    vu_ua_config_t config = {
        .sip_port = 0,           /* Auto-select */
        .rtp_port_start = 4000,
        .rtp_port_count = 100,
        .use_null_audio = true,  /* No sound device by default */
        .log_level = 3
    };
    return config;
}

vu_error_t vu_ua_init(const vu_ua_config_t *config)
{
    if (g_ua.initialized) {
        VU_SET_ERROR(VU_ERR_ALREADY_INITIALIZED, "UA already initialized");
        return VU_ERR_ALREADY_INITIALIZED;
    }

    vu_ua_config_t cfg = config ? *config : vu_ua_default_config();
    pj_status_t status;

    g_ua.state = VU_UA_STATE_INITIALIZING;

    /* Create PJSUA */
    status = pjsua_create();
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_SIP_INIT, status, "pjsua_create failed");
        g_ua.state = VU_UA_STATE_UNINITIALIZED;
        return VU_ERR_SIP_INIT;
    }

    /* Configure PJSUA */
    pjsua_config ua_cfg;
    pjsua_logging_config log_cfg;
    pjsua_media_config media_cfg;

    pjsua_config_default(&ua_cfg);
    pjsua_logging_config_default(&log_cfg);
    pjsua_media_config_default(&media_cfg);

    /* Set callbacks */
    ua_cfg.cb.on_reg_state = on_reg_state;
    ua_cfg.cb.on_incoming_call = on_incoming_call;
    ua_cfg.cb.on_call_state = on_call_state;
    ua_cfg.cb.on_call_media_state = on_call_media_state;
    ua_cfg.cb.on_dtmf_digit2 = on_dtmf_digit2;

    /* Configure logging */
    log_cfg.level = cfg.log_level;
    log_cfg.console_level = cfg.log_level;
    log_cfg.msg_logging = PJ_FALSE;

    /* Configure media */
    media_cfg.clock_rate = 16000;
    media_cfg.snd_clock_rate = 16000;
    media_cfg.no_vad = PJ_TRUE;
    media_cfg.ec_tail_len = 0;

    /* Initialize PJSUA */
    status = pjsua_init(&ua_cfg, &log_cfg, &media_cfg);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_SIP_INIT, status, "pjsua_init failed");
        pjsua_destroy();
        g_ua.state = VU_UA_STATE_UNINITIALIZED;
        return VU_ERR_SIP_INIT;
    }

    /* Add UDP transport - bind to 0.0.0.0 for IPv4 */
    pjsua_transport_config transport_cfg;
    pjsua_transport_config_default(&transport_cfg);
    transport_cfg.port = cfg.sip_port;
    /* Explicitly bind to all IPv4 interfaces */
    transport_cfg.bound_addr = pj_str("0.0.0.0");

    status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &transport_cfg,
                                     &g_ua.udp_transport_id);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_SIP_TRANSPORT, status, "Failed to create UDP transport");
        pjsua_destroy();
        g_ua.state = VU_UA_STATE_UNINITIALIZED;
        return VU_ERR_SIP_TRANSPORT;
    }
    VU_LOG_DEBUG("Created UDP transport with ID %d", g_ua.udp_transport_id);

    /* Set null audio device if requested */
    if (cfg.use_null_audio) {
        status = pjsua_set_null_snd_dev();
        if (status != PJ_SUCCESS) {
            VU_LOG_WARN("Failed to set null audio device: %d", status);
            /* Continue anyway */
        }
    }

    /* Start PJSUA */
    status = pjsua_start();
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_SIP_INIT, status, "pjsua_start failed");
        pjsua_destroy();
        g_ua.state = VU_UA_STATE_UNINITIALIZED;
        return VU_ERR_SIP_INIT;
    }

    /* Get memory pool */
    g_ua.pool = pjsua_pool_create("voip-utility", 4000, 4000);
    if (!g_ua.pool) {
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Failed to create memory pool");
        pjsua_destroy();
        g_ua.state = VU_UA_STATE_UNINITIALIZED;
        return VU_ERR_NO_MEMORY;
    }

    g_ua.state = VU_UA_STATE_RUNNING;
    g_ua.initialized = true;

    VU_LOG_INFO("SIP UA initialized");
    return VU_OK;
}

void vu_ua_shutdown(void)
{
    if (!g_ua.initialized) {
        return;
    }

    VU_LOG_INFO("Shutting down SIP UA");
    g_ua.state = VU_UA_STATE_SHUTTING_DOWN;

    if (g_ua.pool) {
        pj_pool_release(g_ua.pool);
        g_ua.pool = NULL;
    }

    pjsua_destroy();

    g_ua.state = VU_UA_STATE_STOPPED;
    g_ua.initialized = false;
    memset(&g_ua.callbacks, 0, sizeof(g_ua.callbacks));
}

vu_ua_state_t vu_ua_get_state(void)
{
    return g_ua.state;
}

bool vu_ua_is_running(void)
{
    return g_ua.state == VU_UA_STATE_RUNNING;
}

void vu_ua_set_callbacks(const vu_ua_callbacks_t *callbacks)
{
    if (callbacks) {
        g_ua.callbacks = *callbacks;
    } else {
        memset(&g_ua.callbacks, 0, sizeof(g_ua.callbacks));
    }
}

void vu_ua_set_account_manager(vu_account_manager_t *mgr)
{
    g_ua.acc_mgr = mgr;
}

void vu_ua_set_call_manager(vu_call_manager_t *mgr)
{
    g_ua.call_mgr = mgr;
}

int vu_ua_poll(int timeout_ms)
{
    if (!g_ua.initialized) {
        return -1;
    }

    /* PJSUA handles events internally with worker threads,
     * but we can process application-level events here */
    if (timeout_ms > 0) {
        pj_time_val tv;
        tv.sec = timeout_ms / 1000;
        tv.msec = timeout_ms % 1000;
        pj_thread_sleep(timeout_ms);
    }

    return 0;
}

pj_pool_t *vu_ua_get_pool(void)
{
    return g_ua.pool;
}

/* PJSUA Callbacks */

static void on_reg_state(pjsua_acc_id acc_id)
{
    pjsua_acc_info info;
    pjsua_acc_get_info(acc_id, &info);

    VU_LOG_DEBUG("Registration state changed: acc=%d status=%d",
                 acc_id, info.status);

    /* Update account state via account manager */
    if (g_ua.acc_mgr) {
        vu_account_t *account = vu_account_find_by_pjsua_id(g_ua.acc_mgr, acc_id);
        if (account) {
            char reason[128] = "";
            if (info.status_text.slen > 0 && info.status_text.slen < sizeof(reason)) {
                memcpy(reason, info.status_text.ptr, info.status_text.slen);
                reason[info.status_text.slen] = '\0';
            }
            vu_account_on_reg_state(account, info.status, reason);
        }
    }

    if (g_ua.callbacks.on_reg_state) {
        g_ua.callbacks.on_reg_state("", info.status, "");
    }
}

static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
                             pjsip_rx_data *rdata)
{
    (void)rdata;

    pjsua_call_info ci;
    pjsua_call_get_info(call_id, &ci);

    char from_uri[256];
    char to_uri[256];
    pj_memcpy(from_uri, ci.remote_info.ptr,
              ci.remote_info.slen < 255 ? ci.remote_info.slen : 255);
    from_uri[ci.remote_info.slen < 255 ? ci.remote_info.slen : 255] = '\0';
    pj_memcpy(to_uri, ci.local_info.ptr,
              ci.local_info.slen < 255 ? ci.local_info.slen : 255);
    to_uri[ci.local_info.slen < 255 ? ci.local_info.slen : 255] = '\0';

    VU_LOG_INFO("Incoming call: id=%d from=%s to=%s", call_id, from_uri, to_uri);

    if (g_ua.callbacks.on_incoming_call) {
        g_ua.callbacks.on_incoming_call(call_id, from_uri, to_uri);
    }
}

static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
    (void)e;

    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(call_id, &ci);
    if (status != PJ_SUCCESS) {
        VU_LOG_DEBUG("Failed to get call info for call %d", call_id);
        return;
    }

    VU_LOG_DEBUG("Call state changed: id=%d state=%d last_status=%d",
                 call_id, ci.state, ci.last_status);

    /* Update call state via call manager */
    if (g_ua.call_mgr) {
        vu_call_t *call = vu_call_find_by_pjsua_id(g_ua.call_mgr, call_id);
        if (call) {
            vu_call_on_state_change(call, &ci);
        }
    }

    if (g_ua.callbacks.on_call_state) {
        g_ua.callbacks.on_call_state(call_id, ci.state, ci.last_status, "");
    }
}

static void on_call_media_state(pjsua_call_id call_id)
{
    pjsua_call_info ci;
    pj_status_t status = pjsua_call_get_info(call_id, &ci);
    if (status != PJ_SUCCESS) {
        VU_LOG_DEBUG("Failed to get call info for media state, call %d", call_id);
        return;
    }

    VU_LOG_DEBUG("Call media state changed: id=%d media_status=%d",
                 call_id, ci.media_status);

    if (g_ua.callbacks.on_call_media_state) {
        g_ua.callbacks.on_call_media_state(call_id, ci.media_status);
    }

    /* Auto-connect to conference bridge when media is active */
    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        pjsua_conf_connect(ci.conf_slot, 0);
        pjsua_conf_connect(0, ci.conf_slot);
    }
}

static void on_dtmf_digit2(pjsua_call_id call_id, const pjsua_dtmf_info *info)
{
    VU_LOG_DEBUG("DTMF received: id=%d digit=%c duration=%d",
                 call_id, info->digit, info->duration);

    if (g_ua.callbacks.on_dtmf_digit) {
        g_ua.callbacks.on_dtmf_digit(call_id, info->digit, info->duration);
    }
}

pjsua_transport_id vu_ua_get_udp_transport_id(void)
{
    if (!g_ua.initialized) {
        return -1;
    }
    return g_ua.udp_transport_id;
}
