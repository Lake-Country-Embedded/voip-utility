/*
 * voip-utility - SIP VoIP Testing Utility
 * Test definition parser
 */

#ifndef VU_TEST_PARSER_H
#define VU_TEST_PARSER_H

#include "util/error.h"
#include <stdbool.h>

/* Maximum values */
#define VU_MAX_TEST_ACTIONS 32
#define VU_MAX_ACTION_VALUE_LEN 256

/* Action types */
typedef enum {
    VU_ACTION_NONE = 0,
    VU_ACTION_WAIT,           /* Wait N seconds */
    VU_ACTION_SEND_DTMF,      /* Send DTMF digits */
    VU_ACTION_EXPECT_DTMF,    /* Expect DTMF pattern */
    VU_ACTION_PLAY_AUDIO,     /* Play audio file */
    VU_ACTION_RECORD_AUDIO,   /* Start recording */
    VU_ACTION_EXPECT_BEEPS,   /* Expect N beeps */
    VU_ACTION_HANGUP          /* Hangup call */
} vu_action_type_t;

/* Single action */
typedef struct vu_action {
    vu_action_type_t type;
    char value[VU_MAX_ACTION_VALUE_LEN];  /* File path, DTMF digits, etc. */
    int int_value;                         /* Count, timeout, etc. */
    double float_value;                    /* Duration, frequency, etc. */
} vu_action_t;

/* Role configuration (caller or receiver) */
typedef struct vu_role_config {
    char account_id[64];
    char uri[256];           /* Caller only: destination URI */
    bool auto_answer;        /* Receiver only */
    int timeout_sec;         /* Wait timeout */

    vu_action_t actions[VU_MAX_TEST_ACTIONS];
    int action_count;
} vu_role_config_t;

/* Complete test definition */
typedef struct vu_test_definition {
    char name[256];
    char description[512];
    int timeout_sec;         /* Overall test timeout */

    vu_role_config_t caller;
    vu_role_config_t receiver;

    /* Expected results */
    bool expect_connected;
    int expect_beep_count;
    double expect_beep_freq_hz;
} vu_test_definition_t;

/*
 * Parse test definition from JSON file
 * Returns NULL on error (check vu_get_last_error)
 */
vu_test_definition_t *vu_test_parse_file(const char *path);

/*
 * Free test definition
 */
void vu_test_definition_free(vu_test_definition_t *def);

/*
 * Get action type name
 */
const char *vu_action_type_name(vu_action_type_t type);

#endif /* VU_TEST_PARSER_H */
