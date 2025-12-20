/*
 * voip-utility - SIP VoIP Testing Utility
 * Call handling
 */

#ifndef VU_CALL_H
#define VU_CALL_H

#include "util/error.h"
#include "core/account.h"
#include <stdbool.h>
#include <pjsua-lib/pjsua.h>

/* Call state */
typedef enum {
    VU_CALL_STATE_NULL = 0,
    VU_CALL_STATE_CALLING,
    VU_CALL_STATE_INCOMING,
    VU_CALL_STATE_EARLY,
    VU_CALL_STATE_CONNECTING,
    VU_CALL_STATE_CONFIRMED,
    VU_CALL_STATE_DISCONNECTED
} vu_call_state_t;

/* Call media state */
typedef enum {
    VU_CALL_MEDIA_NONE = 0,
    VU_CALL_MEDIA_ACTIVE,
    VU_CALL_MEDIA_LOCAL_HOLD,
    VU_CALL_MEDIA_REMOTE_HOLD,
    VU_CALL_MEDIA_ERROR
} vu_call_media_state_t;

/* Call direction */
typedef enum {
    VU_CALL_DIR_OUTBOUND = 0,
    VU_CALL_DIR_INBOUND
} vu_call_direction_t;

/* Maximum concurrent calls */
#define VU_MAX_CALLS 4

/* Maximum DTMF digits to store */
#define VU_MAX_DTMF_DIGITS 64

/* Call info */
typedef struct vu_call {
    pjsua_call_id pjsua_id;         /* PJSUA call handle (-1 = invalid) */
    vu_call_state_t state;
    vu_call_media_state_t media_state;
    vu_call_direction_t direction;

    char remote_uri[VU_MAX_URI_LEN];
    char local_uri[VU_MAX_URI_LEN];
    char account_id[VU_MAX_URI_LEN];

    uint64_t start_time_ms;          /* Call start time */
    uint64_t connect_time_ms;        /* When call was connected */
    uint64_t end_time_ms;            /* When call ended */

    int last_status_code;
    char last_status_text[256];

    /* Media ports for analysis/recording */
    pjsua_conf_port_id conf_port;    /* Conference bridge port */
    void *analysis_port;             /* Custom audio analysis port */
    void *recorder;                  /* WAV recorder */
    void *player;                    /* Audio player */

    /* DTMF reception buffer */
    char dtmf_buffer[VU_MAX_DTMF_DIGITS + 1];
    int dtmf_count;                  /* Number of digits received */
} vu_call_t;

/* Call manager */
typedef struct vu_call_manager {
    vu_call_t calls[VU_MAX_CALLS];
    int call_count;
} vu_call_manager_t;

/*
 * Initialize call manager
 */
void vu_call_manager_init(vu_call_manager_t *mgr);

/*
 * Cleanup call manager
 */
void vu_call_manager_cleanup(vu_call_manager_t *mgr);

/*
 * Make outbound call
 * Returns call pointer on success, NULL on failure.
 */
vu_call_t *vu_call_make(vu_call_manager_t *mgr, vu_account_t *account,
                         const char *uri);

/*
 * Answer incoming call
 */
vu_error_t vu_call_answer(vu_call_t *call, int code);

/*
 * Hangup call
 */
vu_error_t vu_call_hangup(vu_call_t *call, int code);

/*
 * Hangup all calls
 */
void vu_call_hangup_all(vu_call_manager_t *mgr);

/*
 * Find call by PJSUA call ID
 */
vu_call_t *vu_call_find_by_pjsua_id(vu_call_manager_t *mgr, pjsua_call_id pjsua_id);

/*
 * Find first active call
 */
vu_call_t *vu_call_find_active(vu_call_manager_t *mgr);

/*
 * Wait for call state
 * Returns VU_OK if state reached, VU_ERR_TIMEOUT if timeout.
 */
vu_error_t vu_call_wait_state(vu_call_t *call, vu_call_state_t state, int timeout_sec);

/*
 * Wait for call to connect
 */
vu_error_t vu_call_wait_connected(vu_call_t *call, int timeout_sec);

/*
 * Wait for incoming call
 */
vu_call_t *vu_call_wait_incoming(vu_call_manager_t *mgr, int timeout_sec);

/*
 * Get call state name
 */
const char *vu_call_state_name(vu_call_state_t state);

/*
 * Get call media state name
 */
const char *vu_call_media_state_name(vu_call_media_state_t state);

/*
 * Get call duration in seconds
 */
double vu_call_get_duration(const vu_call_t *call);

/*
 * Update call state from PJSUA callback
 */
void vu_call_on_state_change(vu_call_t *call, pjsua_call_info *ci);

/*
 * Update call media state from PJSUA callback
 */
void vu_call_on_media_state(vu_call_t *call, pjsua_call_info *ci);

/*
 * Handle new incoming call
 */
vu_call_t *vu_call_on_incoming(vu_call_manager_t *mgr, pjsua_call_id call_id,
                                pjsua_call_info *ci);

/*
 * Handle received DTMF digit
 */
void vu_call_on_dtmf_digit(vu_call_t *call, char digit, int duration_ms);

/*
 * Get received DTMF digits
 */
const char *vu_call_get_dtmf_digits(const vu_call_t *call);

/*
 * Clear DTMF buffer
 */
void vu_call_clear_dtmf(vu_call_t *call);

/*
 * Wait for specific DTMF digits
 * Returns VU_OK if pattern matched, VU_ERR_TIMEOUT on timeout.
 */
vu_error_t vu_call_wait_dtmf(vu_call_t *call, const char *pattern, int timeout_sec);

#endif /* VU_CALL_H */
