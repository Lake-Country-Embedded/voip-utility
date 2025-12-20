/*
 * voip-utility - SIP VoIP Testing Utility
 * SIP User Agent (PJSUA wrapper)
 */

#ifndef VU_SIP_UA_H
#define VU_SIP_UA_H

#include "util/error.h"
#include "config/config.h"
#include <stdbool.h>
#include <pjsua-lib/pjsua.h>

/* Forward declarations */
struct vu_call;
struct vu_account;

/* UA state */
typedef enum {
    VU_UA_STATE_UNINITIALIZED = 0,
    VU_UA_STATE_INITIALIZING,
    VU_UA_STATE_RUNNING,
    VU_UA_STATE_SHUTTING_DOWN,
    VU_UA_STATE_STOPPED
} vu_ua_state_t;

/* Callback types */
typedef void (*vu_ua_on_reg_state_cb)(const char *account_id, int code, const char *reason);
typedef void (*vu_ua_on_incoming_call_cb)(int call_id, const char *from_uri, const char *to_uri);
typedef void (*vu_ua_on_call_state_cb)(int call_id, int state, int code, const char *reason);
typedef void (*vu_ua_on_call_media_state_cb)(int call_id, int media_state);
typedef void (*vu_ua_on_dtmf_digit_cb)(int call_id, char digit, int duration_ms);

/* UA callbacks */
typedef struct vu_ua_callbacks {
    vu_ua_on_reg_state_cb on_reg_state;
    vu_ua_on_incoming_call_cb on_incoming_call;
    vu_ua_on_call_state_cb on_call_state;
    vu_ua_on_call_media_state_cb on_call_media_state;
    vu_ua_on_dtmf_digit_cb on_dtmf_digit;
    void *user_data;
} vu_ua_callbacks_t;

/* UA configuration */
typedef struct vu_ua_config {
    uint16_t sip_port;              /* Local SIP port (0 = auto) */
    uint16_t rtp_port_start;        /* RTP port range start (default 4000) */
    uint16_t rtp_port_count;        /* Number of RTP ports (default 100) */
    bool use_null_audio;            /* Use null audio device (no sound) */
    uint32_t log_level;             /* PJSIP log level (0-6) */
} vu_ua_config_t;

/*
 * Get default UA configuration
 */
vu_ua_config_t vu_ua_default_config(void);

/*
 * Initialize the SIP User Agent
 */
vu_error_t vu_ua_init(const vu_ua_config_t *config);

/*
 * Shutdown the SIP User Agent
 */
void vu_ua_shutdown(void);

/*
 * Get current UA state
 */
vu_ua_state_t vu_ua_get_state(void);

/*
 * Check if UA is initialized and running
 */
bool vu_ua_is_running(void);

/*
 * Set callbacks
 */
void vu_ua_set_callbacks(const vu_ua_callbacks_t *callbacks);

/*
 * Set account manager for registration callbacks
 */
struct vu_account_manager;
void vu_ua_set_account_manager(struct vu_account_manager *mgr);

/*
 * Set call manager for call state callbacks
 */
struct vu_call_manager;
void vu_ua_set_call_manager(struct vu_call_manager *mgr);

/*
 * Process events (poll-based, use in main loop)
 * Returns number of events processed, or -1 on error.
 * timeout_ms: 0 = no wait, -1 = wait forever
 */
int vu_ua_poll(int timeout_ms);

/*
 * Get PJSUA pool (for memory allocation)
 */
pj_pool_t *vu_ua_get_pool(void);

#endif /* VU_SIP_UA_H */
