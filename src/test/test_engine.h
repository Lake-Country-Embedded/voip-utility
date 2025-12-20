/*
 * voip-utility - SIP VoIP Testing Utility
 * Test execution engine
 */

#ifndef VU_TEST_ENGINE_H
#define VU_TEST_ENGINE_H

#include "util/error.h"
#include "config/config.h"
#include "test/test_parser.h"
#include "core/call.h"

/* Test result status */
typedef enum {
    VU_TEST_PENDING = 0,
    VU_TEST_RUNNING,
    VU_TEST_PASSED,
    VU_TEST_FAILED,
    VU_TEST_TIMEOUT,
    VU_TEST_ERROR
} vu_test_status_t;

/* Test result */
typedef struct vu_test_result {
    vu_test_status_t status;
    double duration_sec;
    char error_message[512];

    /* Detailed results */
    bool connected;
    int dtmf_received_count;
    int beeps_detected;
    double beep_frequency;
    char dtmf_received[VU_MAX_DTMF_DIGITS + 1];
} vu_test_result_t;

/* Test engine state */
typedef struct vu_test_engine vu_test_engine_t;

/*
 * Create test engine
 */
vu_test_engine_t *vu_test_engine_create(const vu_config_t *config);

/*
 * Destroy test engine
 */
void vu_test_engine_destroy(vu_test_engine_t *engine);

/*
 * Load test from file
 */
vu_error_t vu_test_engine_load(vu_test_engine_t *engine, const char *test_file);

/*
 * Run loaded test
 */
vu_error_t vu_test_engine_run(vu_test_engine_t *engine);

/*
 * Get test result
 */
const vu_test_result_t *vu_test_engine_get_result(const vu_test_engine_t *engine);

/*
 * Get test status name
 */
const char *vu_test_status_name(vu_test_status_t status);

#endif /* VU_TEST_ENGINE_H */
