/*
 * voip-utility - SIP VoIP Testing Utility
 * Call command implementation
 */

#include "cli/cli.h"
#include "core/sip_ua.h"
#include "core/account.h"
#include "core/call.h"
#include "core/dtmf.h"
#include "core/media.h"
#include "util/log.h"
#include "util/json_output.h"
#include "util/time_util.h"

extern int vu_is_running(void);

/* Global state for callbacks */
static vu_call_manager_t *g_call_mgr = NULL;

static void on_dtmf_digit(int call_id, char digit, int duration_ms)
{
    if (!g_call_mgr) return;

    vu_call_t *call = vu_call_find_by_pjsua_id(g_call_mgr, call_id);
    if (call) {
        vu_call_on_dtmf_digit(call, digit, duration_ms);
    }
}

int vu_cmd_call(const vu_cli_args_t *args, vu_config_t *config)
{
    if (!args || !config) return 1;

    const vu_call_opts_t *opts = &args->cmd.call;

    if (!opts->uri) {
        VU_LOG_ERROR("URI is required. Use -u <uri>");
        return 1;
    }

    /* Find account */
    vu_account_config_t *acc_cfg = NULL;
    if (opts->account_id) {
        acc_cfg = vu_config_find_account(config, opts->account_id);
        if (!acc_cfg) {
            VU_LOG_ERROR("Account not found: %s", opts->account_id);
            return 1;
        }
    } else if (config->account_count > 0) {
        acc_cfg = &config->accounts[0];
    } else {
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

    /* Initialize account manager and register */
    vu_account_manager_t acc_mgr;
    vu_account_manager_init(&acc_mgr, NULL);
    vu_account_add(&acc_mgr, acc_cfg);

    /* Set account manager on UA for callbacks */
    vu_ua_set_account_manager(&acc_mgr);

    vu_account_t *account = &acc_mgr.accounts[0];
    err = vu_account_register(account);
    if (err != VU_OK) {
        VU_LOG_ERROR("Failed to register: %s", vu_error_str(err));
        vu_ua_shutdown();
        return 1;
    }

    err = vu_account_wait_registration(account, 30);
    if (err != VU_OK) {
        VU_LOG_ERROR("Registration failed: %s", vu_error_str(err));
        vu_ua_shutdown();
        return 1;
    }

    /* Set up DTMF callback */
    vu_ua_callbacks_t callbacks = {0};
    callbacks.on_dtmf_digit = on_dtmf_digit;
    vu_ua_set_callbacks(&callbacks);

    /* Initialize call manager */
    vu_call_manager_t call_mgr;
    vu_call_manager_init(&call_mgr);
    vu_ua_set_call_manager(&call_mgr);
    g_call_mgr = &call_mgr;

    int result = 0;

    /* Make call */
    if (args->global.json_output) {
        vu_json_output(vu_json_event_calling(opts->uri, account->config.id));
    }

    vu_call_t *call = vu_call_make(&call_mgr, account, opts->uri);
    if (!call) {
        VU_LOG_ERROR("Failed to make call: %s", vu_get_last_error()->message);
        result = 1;
        goto cleanup;
    }

    /* Wait for connect */
    err = vu_call_wait_connected(call, opts->timeout_sec);
    if (err != VU_OK) {
        VU_LOG_ERROR("Call failed: %s", vu_error_str(err));
        result = 1;
        goto cleanup;
    }

    VU_LOG_INFO("Call connected");
    if (args->global.json_output) {
        double connect_time = (double)(call->connect_time_ms - call->start_time_ms) / 1000.0;
        vu_json_output(vu_json_event_call_connected(call->pjsua_id, connect_time));
    }

    /* Start recording if requested */
    if (opts->record_path) {
        vu_media_start_recording(call, opts->record_path);
    }

    /* Play audio immediately if no delay specified */
    if (opts->play_file && opts->play_delay_ms <= 0) {
        vu_media_play_file(call, opts->play_file, false);
    }

    /* Send DTMF if requested */
    if (opts->dtmf) {
        VU_LOG_DEBUG("Waiting %d ms before sending DTMF", opts->dtmf_delay_ms);
        vu_sleep_ms(opts->dtmf_delay_ms);
        vu_dtmf_send(call, opts->dtmf, NULL);
        if (args->global.json_output) {
            vu_json_output(vu_json_event_dtmf_sent(call->pjsua_id, opts->dtmf));
        }
    }

    /* Play audio after delay if specified */
    if (opts->play_file && opts->play_delay_ms > 0) {
        VU_LOG_DEBUG("Waiting %d ms before playing audio", opts->play_delay_ms);
        vu_sleep_ms(opts->play_delay_ms);
        vu_media_play_file(call, opts->play_file, false);
    }

    /* Wait for hangup or timeout */
    if (opts->hangup_after_sec > 0) {
        vu_timer_t timer;
        vu_timer_start(&timer, opts->hangup_after_sec * 1000);
        while (!vu_timer_expired(&timer) && vu_is_running() &&
               call->state == VU_CALL_STATE_CONFIRMED) {
            vu_ua_poll(100);
        }
    } else {
        /* Wait until remote hangup or Ctrl+C */
        while (vu_is_running() && call->state == VU_CALL_STATE_CONFIRMED) {
            vu_ua_poll(100);
        }
    }

    /* Hangup */
    vu_call_hangup(call, 200);

    if (args->global.json_output) {
        vu_json_output(vu_json_event_call_disconnected(call->pjsua_id, 200, "Normal",
                                                        vu_call_get_duration(call)));
    }

cleanup:
    g_call_mgr = NULL;
    /* Clear managers from UA before cleanup to prevent callback races */
    vu_ua_set_call_manager(NULL);
    vu_ua_set_account_manager(NULL);
    vu_call_manager_cleanup(&call_mgr);
    vu_account_manager_cleanup(&acc_mgr);
    vu_ua_shutdown();
    return result;
}
