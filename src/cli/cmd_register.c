/*
 * voip-utility - SIP VoIP Testing Utility
 * Register command implementation
 */

#include "cli/cli.h"
#include "core/sip_ua.h"
#include "core/account.h"
#include "util/log.h"
#include "util/json_output.h"

/* External running flag */
extern int vu_is_running(void);

int vu_cmd_register(const vu_cli_args_t *args, vu_config_t *config)
{
    if (!args || !config) return 1;

    const vu_register_opts_t *opts = &args->cmd.reg;

    /* Check we have accounts */
    if (config->account_count == 0) {
        VU_LOG_ERROR("No accounts configured");
        return 1;
    }

    /* Initialize UA */
    vu_ua_config_t ua_cfg = vu_ua_default_config();
    vu_error_t err = vu_ua_init(&ua_cfg);
    if (err != VU_OK) {
        VU_LOG_ERROR("Failed to initialize SIP UA: %s", vu_error_str(err));
        return 1;
    }

    /* Initialize account manager */
    vu_account_manager_t acc_mgr;
    err = vu_account_manager_init(&acc_mgr, config);
    if (err != VU_OK) {
        VU_LOG_ERROR("Failed to initialize accounts: %s", vu_error_str(err));
        vu_ua_shutdown();
        return 1;
    }

    /* Set account manager on UA for callbacks */
    vu_ua_set_account_manager(&acc_mgr);

    int result = 0;

    if (opts->account_id) {
        /* Register specific account */
        vu_account_t *acc = vu_account_find(&acc_mgr, opts->account_id);
        if (!acc) {
            VU_LOG_ERROR("Account not found: %s", opts->account_id);
            result = 1;
            goto cleanup;
        }

        if (opts->unregister) {
            err = vu_account_unregister(acc);
        } else {
            err = vu_account_register(acc);
            if (err == VU_OK) {
                err = vu_account_wait_registration(acc, opts->timeout_sec);
            }
        }

        if (err != VU_OK) {
            VU_LOG_ERROR("Registration failed: %s", vu_error_str(err));
            result = 1;
        } else {
            VU_LOG_INFO("Account %s: %s", acc->config.id,
                        vu_account_state_name(acc->state));
        }
    } else {
        /* Register all enabled accounts */
        if (opts->unregister) {
            vu_account_unregister_all(&acc_mgr);
        } else {
            err = vu_account_register_all(&acc_mgr);
            if (err != VU_OK) {
                VU_LOG_WARN("Some registrations failed");
            }

            /* Wait for all to complete */
            for (int i = 0; i < acc_mgr.account_count && vu_is_running(); i++) {
                vu_account_t *acc = &acc_mgr.accounts[i];
                if (acc->config.enabled && acc->state == VU_ACCOUNT_STATE_REGISTERING) {
                    vu_account_wait_registration(acc, opts->timeout_sec);
                }
            }

            /* Print status */
            for (int i = 0; i < acc_mgr.account_count; i++) {
                vu_account_t *acc = &acc_mgr.accounts[i];
                VU_LOG_INFO("Account %s: %s", acc->config.id,
                            vu_account_state_name(acc->state));

                if (args->global.json_output) {
                    cJSON *json = vu_json_event_registered(acc->config.id,
                                                            acc->config.reg_timeout_sec);
                    vu_json_output(json);
                }
            }
        }
    }

cleanup:
    /* Clear managers from UA before cleanup to prevent callback races */
    vu_ua_set_account_manager(NULL);
    vu_account_manager_cleanup(&acc_mgr);
    vu_ua_shutdown();
    return result;
}
