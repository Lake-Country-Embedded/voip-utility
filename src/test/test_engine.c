/*
 * voip-utility - SIP VoIP Testing Utility
 * Test execution engine implementation
 */

#include "test/test_engine.h"
#include "test/test_parser.h"
#include "core/sip_ua.h"
#include "core/account.h"
#include "core/call.h"
#include "core/dtmf.h"
#include "core/media.h"
#include "audio/analyzer.h"
#include "audio/beep_detector.h"
#include "util/log.h"
#include "util/time_util.h"
#include <stdlib.h>
#include <string.h>

extern int vu_is_running(void);

struct vu_test_engine {
    const vu_config_t *config;
    vu_test_definition_t *test_def;
    vu_test_result_t result;

    vu_account_manager_t acc_mgr;
    vu_call_manager_t call_mgr;

    vu_account_t *caller_account;
    vu_account_t *receiver_account;
    vu_call_t *caller_call;
    vu_call_t *receiver_call;

    int caller_action_index;
    int receiver_action_index;
    uint64_t test_start_time_ms;
};

const char *vu_test_status_name(vu_test_status_t status)
{
    switch (status) {
    case VU_TEST_PENDING:  return "pending";
    case VU_TEST_RUNNING:  return "running";
    case VU_TEST_PASSED:   return "passed";
    case VU_TEST_FAILED:   return "failed";
    case VU_TEST_TIMEOUT:  return "timeout";
    case VU_TEST_ERROR:    return "error";
    default:               return "unknown";
    }
}

/* Callback for incoming calls */
static vu_test_engine_t *g_engine = NULL;

static void on_incoming_call(int call_id, const char *from_uri, const char *to_uri)
{
    if (!g_engine) return;

    VU_LOG_INFO("Test: Incoming call from %s to %s", from_uri, to_uri);

    vu_call_t *call = vu_call_find_by_pjsua_id(&g_engine->call_mgr, call_id);
    if (!call) {
        pjsua_call_info ci;
        pjsua_call_get_info(call_id, &ci);
        call = vu_call_on_incoming(&g_engine->call_mgr, call_id, &ci);
    }

    if (call && g_engine->test_def) {
        g_engine->receiver_call = call;

        if (g_engine->test_def->receiver.auto_answer) {
            VU_LOG_INFO("Test: Auto-answering call");
            vu_call_answer(call, 200);
        }
    }
}

static void on_dtmf_digit(int call_id, char digit, int duration_ms)
{
    if (!g_engine) return;

    vu_call_t *call = vu_call_find_by_pjsua_id(&g_engine->call_mgr, call_id);
    if (call) {
        vu_call_on_dtmf_digit(call, digit, duration_ms);
    }
}

vu_test_engine_t *vu_test_engine_create(const vu_config_t *config)
{
    vu_test_engine_t *engine = calloc(1, sizeof(vu_test_engine_t));
    if (!engine) return NULL;

    engine->config = config;
    engine->result.status = VU_TEST_PENDING;

    return engine;
}

void vu_test_engine_destroy(vu_test_engine_t *engine)
{
    if (!engine) return;

    if (engine->test_def) {
        vu_test_definition_free(engine->test_def);
    }

    free(engine);
}

vu_error_t vu_test_engine_load(vu_test_engine_t *engine, const char *test_file)
{
    if (!engine || !test_file) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Invalid arguments");
        return VU_ERR_INVALID_ARG;
    }

    if (engine->test_def) {
        vu_test_definition_free(engine->test_def);
    }

    engine->test_def = vu_test_parse_file(test_file);
    if (!engine->test_def) {
        return VU_ERR_CONFIG_PARSE;
    }

    return VU_OK;
}

/* Execute a single action */
static vu_error_t execute_action(vu_test_engine_t *engine, vu_call_t *call,
                                  const vu_action_t *action)
{
    if (!action || action->type == VU_ACTION_NONE) {
        return VU_OK;
    }

    VU_LOG_INFO("Test: Executing action '%s'", vu_action_type_name(action->type));

    switch (action->type) {
    case VU_ACTION_WAIT:
        /* Use vu_ua_poll to process PJSIP events while waiting */
        {
            int wait_ms = (int)(action->float_value * 1000);
            while (wait_ms > 0 && vu_is_running()) {
                int poll_ms = (wait_ms > 100) ? 100 : wait_ms;
                vu_ua_poll(poll_ms);
                wait_ms -= poll_ms;
            }
        }
        break;

    case VU_ACTION_SEND_DTMF:
        if (call && action->value[0]) {
            vu_dtmf_send(call, action->value, NULL);
        }
        break;

    case VU_ACTION_EXPECT_DTMF:
        /* DTMF verification is done post-call - skip during action execution */
        break;

    case VU_ACTION_PLAY_AUDIO:
        if (call && action->value[0]) {
            vu_media_play_file(call, action->value, action->int_value != 0);
        }
        break;

    case VU_ACTION_RECORD_AUDIO:
        if (call && action->value[0]) {
            vu_media_start_recording(call, action->value);
        }
        break;

    case VU_ACTION_EXPECT_BEEPS:
        /* Beep detection is done post-call on recorded audio */
        break;

    case VU_ACTION_HANGUP:
        if (call) {
            vu_call_hangup(call, action->int_value ? action->int_value : 200);
        }
        break;

    default:
        break;
    }

    return VU_OK;
}

vu_error_t vu_test_engine_run(vu_test_engine_t *engine)
{
    if (!engine || !engine->test_def) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "No test loaded");
        return VU_ERR_INVALID_ARG;
    }

    vu_test_definition_t *def = engine->test_def;
    engine->result.status = VU_TEST_RUNNING;
    engine->test_start_time_ms = vu_time_now_ms();

    VU_LOG_INFO("=== Running test: %s ===", def->name);

    /* Initialize UA */
    vu_ua_config_t ua_cfg = vu_ua_default_config();
    vu_error_t err = vu_ua_init(&ua_cfg);
    if (err != VU_OK) {
        engine->result.status = VU_TEST_ERROR;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Failed to initialize SIP UA");
        return err;
    }

    /* Set up callbacks */
    g_engine = engine;
    vu_ua_callbacks_t callbacks = {0};
    callbacks.on_incoming_call = on_incoming_call;
    callbacks.on_dtmf_digit = on_dtmf_digit;
    vu_ua_set_callbacks(&callbacks);

    /* Initialize managers */
    vu_account_manager_init(&engine->acc_mgr, NULL);
    vu_call_manager_init(&engine->call_mgr);
    vu_ua_set_account_manager(&engine->acc_mgr);
    vu_ua_set_call_manager(&engine->call_mgr);

    /* Find and register accounts */
    vu_account_config_t *caller_cfg = vu_config_find_account((vu_config_t *)engine->config,
                                                              def->caller.account_id);
    vu_account_config_t *receiver_cfg = vu_config_find_account((vu_config_t *)engine->config,
                                                                def->receiver.account_id);

    if (!caller_cfg || !receiver_cfg) {
        engine->result.status = VU_TEST_ERROR;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Account not found: caller=%s receiver=%s",
                def->caller.account_id, def->receiver.account_id);
        goto cleanup;
    }

    vu_account_add(&engine->acc_mgr, receiver_cfg);
    vu_account_add(&engine->acc_mgr, caller_cfg);

    engine->receiver_account = &engine->acc_mgr.accounts[0];
    engine->caller_account = &engine->acc_mgr.accounts[1];

    /* Register accounts */
    err = vu_account_register(engine->receiver_account);
    if (err != VU_OK) {
        engine->result.status = VU_TEST_ERROR;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Failed to register receiver account");
        goto cleanup;
    }

    err = vu_account_register(engine->caller_account);
    if (err != VU_OK) {
        engine->result.status = VU_TEST_ERROR;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Failed to register caller account");
        goto cleanup;
    }

    /* Wait for registrations */
    err = vu_account_wait_registration(engine->receiver_account, 10);
    if (err != VU_OK) {
        engine->result.status = VU_TEST_ERROR;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Receiver registration failed");
        goto cleanup;
    }

    err = vu_account_wait_registration(engine->caller_account, 10);
    if (err != VU_OK) {
        engine->result.status = VU_TEST_ERROR;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Caller registration failed");
        goto cleanup;
    }

    VU_LOG_INFO("Test: Both accounts registered");

    /* Make the call */
    engine->caller_call = vu_call_make(&engine->call_mgr, engine->caller_account,
                                        def->caller.uri);
    if (!engine->caller_call) {
        engine->result.status = VU_TEST_ERROR;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Failed to make call");
        goto cleanup;
    }

    /* Wait for call to connect */
    err = vu_call_wait_connected(engine->caller_call, def->caller.timeout_sec);
    if (err != VU_OK) {
        engine->result.status = VU_TEST_FAILED;
        engine->result.connected = false;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Call failed to connect");
        goto cleanup;
    }

    engine->result.connected = true;
    VU_LOG_INFO("Test: Call connected");

    /* Execute receiver actions first (recording, etc.) */
    for (int i = 0; i < def->receiver.action_count; i++) {
        err = execute_action(engine, engine->receiver_call, &def->receiver.actions[i]);
        if (err != VU_OK) {
            engine->result.status = VU_TEST_FAILED;
            goto cleanup;
        }
    }

    /* Execute caller actions */
    for (int i = 0; i < def->caller.action_count; i++) {
        if (!vu_is_running()) break;

        err = execute_action(engine, engine->caller_call, &def->caller.actions[i]);
        if (err != VU_OK) {
            engine->result.status = VU_TEST_FAILED;
            goto cleanup;
        }
    }

    /* Wait a moment for things to settle */
    vu_ua_poll(500);

    /* Collect DTMF results */
    if (engine->receiver_call) {
        const char *dtmf = vu_call_get_dtmf_digits(engine->receiver_call);
        if (dtmf && dtmf[0]) {
            strncpy(engine->result.dtmf_received, dtmf, VU_MAX_DTMF_DIGITS);
            engine->result.dtmf_received[VU_MAX_DTMF_DIGITS] = '\0';
            engine->result.dtmf_received_count = strlen(dtmf);
        }
    }

    /* Analyze recording for beeps if needed */
    if (def->expect_beep_count > 0) {
        /* Find recording path from receiver actions */
        const char *recording_path = NULL;
        for (int i = 0; i < def->receiver.action_count; i++) {
            if (def->receiver.actions[i].type == VU_ACTION_RECORD_AUDIO) {
                recording_path = def->receiver.actions[i].value;
                break;
            }
        }

        if (recording_path && recording_path[0]) {
            VU_LOG_INFO("Test: Analyzing recording %s for beeps", recording_path);

            vu_analyzer_config_t analyzer_cfg = vu_analyzer_default_config();
            size_t frame_count = 0;
            vu_freq_result_t *results = vu_analyzer_analyze_file(recording_path,
                                                                  &analyzer_cfg, &frame_count);

            if (results && frame_count > 0) {
                /* PJSUA is configured for 16kHz, so recordings are 16kHz */
                const unsigned sample_rate = 16000;
                vu_beep_config_t beep_cfg = engine->config->beep;
                vu_beep_detector_t *detector = vu_beep_detector_create(&beep_cfg, sample_rate);

                if (detector) {
                    double frame_duration = (double)analyzer_cfg.fft_size / 2 / (double)sample_rate;
                    vu_level_result_t level = {0};

                    for (size_t i = 0; i < frame_count; i++) {
                        double time = i * frame_duration;
                        level.rms_db = results[i].magnitude_db;
                        level.is_silence = !results[i].valid;

                        vu_beep_event_t event;
                        vu_beep_detector_process(detector, &results[i], &level, time, &event);
                    }

                    const vu_beep_result_t *beep_result = vu_beep_detector_get_result(detector);
                    engine->result.beeps_detected = beep_result->valid_beep_count;

                    if (beep_result->beeps && beep_result->valid_beep_count > 0) {
                        engine->result.beep_frequency = beep_result->beeps[0].frequency_hz;
                    }

                    VU_LOG_INFO("Test: Detected %d beeps", engine->result.beeps_detected);
                    vu_beep_detector_destroy(detector);
                }

                vu_analyzer_free_results(results);
            }
        }
    }

    /* Evaluate test results */
    engine->result.duration_sec = (double)(vu_time_now_ms() - engine->test_start_time_ms) / 1000.0;

    bool test_passed = true;

    if (def->expect_connected && !engine->result.connected) {
        test_passed = false;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Expected call to connect but it didn't");
    }

    if (def->expect_beep_count > 0 && engine->result.beeps_detected != def->expect_beep_count) {
        test_passed = false;
        snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                "Expected %d beeps, detected %d",
                def->expect_beep_count, engine->result.beeps_detected);
    }

    /* Check DTMF expectations from receiver actions */
    for (int i = 0; i < def->receiver.action_count && test_passed; i++) {
        if (def->receiver.actions[i].type == VU_ACTION_EXPECT_DTMF) {
            const char *expected = def->receiver.actions[i].value;
            const char *received = engine->result.dtmf_received;

            if (!received || strlen(received) < strlen(expected) ||
                strncmp(received, expected, strlen(expected)) != 0) {
                test_passed = false;
                snprintf(engine->result.error_message, sizeof(engine->result.error_message),
                        "Expected DTMF '%s', received '%s'",
                        expected, received ? received : "(none)");
            } else {
                VU_LOG_INFO("Test: DTMF pattern '%s' verified", expected);
            }
        }
    }

    engine->result.status = test_passed ? VU_TEST_PASSED : VU_TEST_FAILED;

cleanup:
    g_engine = NULL;

    /* Hangup any active calls */
    vu_call_hangup_all(&engine->call_mgr);
    vu_ua_poll(500);

    /* Clear managers */
    vu_ua_set_call_manager(NULL);
    vu_ua_set_account_manager(NULL);
    vu_call_manager_cleanup(&engine->call_mgr);
    vu_account_manager_cleanup(&engine->acc_mgr);
    vu_ua_shutdown();

    engine->result.duration_sec = (double)(vu_time_now_ms() - engine->test_start_time_ms) / 1000.0;

    VU_LOG_INFO("=== Test %s: %s (%.2fs) ===",
               def->name,
               vu_test_status_name(engine->result.status),
               engine->result.duration_sec);

    if (engine->result.error_message[0]) {
        VU_LOG_INFO("Error: %s", engine->result.error_message);
    }

    return engine->result.status == VU_TEST_PASSED ? VU_OK : VU_ERR_TEST_FAILED;
}

const vu_test_result_t *vu_test_engine_get_result(const vu_test_engine_t *engine)
{
    if (!engine) return NULL;
    return &engine->result;
}
