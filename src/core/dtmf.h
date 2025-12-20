/*
 * voip-utility - SIP VoIP Testing Utility
 * DTMF handling
 */

#ifndef VU_DTMF_H
#define VU_DTMF_H

#include "util/error.h"
#include "core/call.h"

/* DTMF method */
typedef enum {
    VU_DTMF_RFC2833 = 0,    /* RFC 2833 (telephone-events) - preferred */
    VU_DTMF_INBAND,         /* In-band audio tones */
    VU_DTMF_SIP_INFO        /* SIP INFO messages */
} vu_dtmf_method_t;

/* DTMF send options */
typedef struct vu_dtmf_opts {
    vu_dtmf_method_t method;
    int duration_ms;         /* Tone duration (default 100) */
    int gap_ms;              /* Gap between digits (default 100) */
} vu_dtmf_opts_t;

/*
 * Get default DTMF options
 */
vu_dtmf_opts_t vu_dtmf_default_opts(void);

/*
 * Send DTMF digits on call
 */
vu_error_t vu_dtmf_send(vu_call_t *call, const char *digits, const vu_dtmf_opts_t *opts);

/*
 * Send single DTMF digit
 */
vu_error_t vu_dtmf_send_digit(vu_call_t *call, char digit, const vu_dtmf_opts_t *opts);

/*
 * Get DTMF method name
 */
const char *vu_dtmf_method_name(vu_dtmf_method_t method);

/*
 * Parse DTMF method from string
 */
vu_dtmf_method_t vu_dtmf_method_from_string(const char *str);

#endif /* VU_DTMF_H */
