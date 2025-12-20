/*
 * voip-utility - SIP VoIP Testing Utility
 * Error handling definitions
 */

#ifndef VU_ERROR_H
#define VU_ERROR_H

#include <stddef.h>

/* Error codes */
typedef enum {
    VU_OK = 0,

    /* General errors (-1 to -99) */
    VU_ERR_INVALID_ARG = -1,
    VU_ERR_NO_MEMORY = -2,
    VU_ERR_NOT_FOUND = -3,
    VU_ERR_ALREADY_EXISTS = -4,
    VU_ERR_NOT_INITIALIZED = -5,
    VU_ERR_ALREADY_INITIALIZED = -6,
    VU_ERR_IO = -7,
    VU_ERR_TIMEOUT = -8,
    VU_ERR_CANCELLED = -9,
    VU_ERR_BUSY = -10,

    /* SIP/PJSIP errors (-100 to -199) */
    VU_ERR_SIP_INIT = -100,
    VU_ERR_SIP_TRANSPORT = -101,
    VU_ERR_SIP_ACCOUNT = -102,
    VU_ERR_REGISTRATION_FAILED = -103,
    VU_ERR_CALL_FAILED = -104,
    VU_ERR_CALL_NOT_ACTIVE = -105,
    VU_ERR_CALL_REJECTED = -106,
    VU_ERR_CALL_TIMEOUT = -107,
    VU_ERR_NO_ACCOUNT = -108,

    /* Media errors (-200 to -299) */
    VU_ERR_MEDIA_INIT = -200,
    VU_ERR_MEDIA_CONNECT = -201,
    VU_ERR_MEDIA_CODEC = -202,
    VU_ERR_FILE_OPEN = -203,
    VU_ERR_FILE_FORMAT = -204,
    VU_ERR_AUDIO_DEVICE = -205,
    VU_ERR_MEDIA_ERROR = -206,

    /* Test errors (-300 to -399) */
    VU_ERR_TEST_PARSE = -300,
    VU_ERR_TEST_INVALID = -301,
    VU_ERR_TEST_TIMEOUT = -302,
    VU_ERR_TEST_CONDITION_FAILED = -303,
    VU_ERR_EVENT_NOT_MATCHED = -304,
    VU_ERR_ACTION_FAILED = -305,
    VU_ERR_TEST_FAILED = -306,

    /* Config errors (-400 to -499) */
    VU_ERR_CONFIG_PARSE = -400,
    VU_ERR_CONFIG_INVALID = -401,
    VU_ERR_CONFIG_NOT_FOUND = -402,
} vu_error_t;

/* Thread-local error context */
typedef struct vu_error_context {
    vu_error_t code;
    char message[512];
    const char *file;
    int line;
    int pjsip_status;  /* Original PJSIP status if applicable */
} vu_error_context_t;

/*
 * Get string description of error code
 */
const char *vu_error_str(vu_error_t err);

/*
 * Get last error context (thread-local)
 */
vu_error_context_t *vu_get_last_error(void);

/*
 * Clear last error
 */
void vu_clear_error(void);

/*
 * Set error with context (internal use via macro)
 */
void vu_set_error_internal(vu_error_t err, const char *file, int line,
                           const char *fmt, ...);

/*
 * Set error with PJSIP status
 */
void vu_set_pjsip_error(vu_error_t err, int pjsip_status,
                        const char *file, int line, const char *fmt, ...);

/* Convenience macros */
#define VU_SET_ERROR(err, fmt, ...) \
    vu_set_error_internal(err, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define VU_SET_PJSIP_ERROR(err, status, fmt, ...) \
    vu_set_pjsip_error(err, status, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/* Check and return on error */
#define VU_CHECK(expr) \
    do { \
        vu_error_t _err = (expr); \
        if (_err != VU_OK) return _err; \
    } while (0)

/* Check with cleanup */
#define VU_CHECK_GOTO(expr, label) \
    do { \
        vu_error_t _err = (expr); \
        if (_err != VU_OK) { \
            err = _err; \
            goto label; \
        } \
    } while (0)

#endif /* VU_ERROR_H */
