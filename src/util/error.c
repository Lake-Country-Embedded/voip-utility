/*
 * voip-utility - SIP VoIP Testing Utility
 * Error handling implementation
 */

#include "util/error.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

/* Thread-local error context */
static __thread vu_error_context_t tls_error_ctx = {0};

const char *vu_error_str(vu_error_t err)
{
    switch (err) {
    case VU_OK:
        return "Success";

    /* General errors */
    case VU_ERR_INVALID_ARG:
        return "Invalid argument";
    case VU_ERR_NO_MEMORY:
        return "Out of memory";
    case VU_ERR_NOT_FOUND:
        return "Not found";
    case VU_ERR_ALREADY_EXISTS:
        return "Already exists";
    case VU_ERR_NOT_INITIALIZED:
        return "Not initialized";
    case VU_ERR_ALREADY_INITIALIZED:
        return "Already initialized";
    case VU_ERR_IO:
        return "I/O error";
    case VU_ERR_TIMEOUT:
        return "Timeout";
    case VU_ERR_CANCELLED:
        return "Cancelled";
    case VU_ERR_BUSY:
        return "Busy";

    /* SIP errors */
    case VU_ERR_SIP_INIT:
        return "SIP initialization failed";
    case VU_ERR_SIP_TRANSPORT:
        return "SIP transport error";
    case VU_ERR_SIP_ACCOUNT:
        return "SIP account error";
    case VU_ERR_REGISTRATION_FAILED:
        return "SIP registration failed";
    case VU_ERR_CALL_FAILED:
        return "Call failed";
    case VU_ERR_CALL_NOT_ACTIVE:
        return "Call not active";
    case VU_ERR_CALL_REJECTED:
        return "Call rejected";
    case VU_ERR_CALL_TIMEOUT:
        return "Call timeout";
    case VU_ERR_NO_ACCOUNT:
        return "No account configured";

    /* Media errors */
    case VU_ERR_MEDIA_INIT:
        return "Media initialization failed";
    case VU_ERR_MEDIA_CONNECT:
        return "Media connection failed";
    case VU_ERR_MEDIA_CODEC:
        return "Codec error";
    case VU_ERR_FILE_OPEN:
        return "Failed to open file";
    case VU_ERR_FILE_FORMAT:
        return "Invalid file format";
    case VU_ERR_AUDIO_DEVICE:
        return "Audio device error";

    /* Test errors */
    case VU_ERR_TEST_PARSE:
        return "Test parse error";
    case VU_ERR_TEST_INVALID:
        return "Invalid test definition";
    case VU_ERR_TEST_TIMEOUT:
        return "Test timeout";
    case VU_ERR_TEST_CONDITION_FAILED:
        return "Test condition failed";
    case VU_ERR_EVENT_NOT_MATCHED:
        return "Expected event not matched";
    case VU_ERR_ACTION_FAILED:
        return "Action execution failed";

    /* Config errors */
    case VU_ERR_CONFIG_PARSE:
        return "Configuration parse error";
    case VU_ERR_CONFIG_INVALID:
        return "Invalid configuration";
    case VU_ERR_CONFIG_NOT_FOUND:
        return "Configuration not found";

    default:
        return "Unknown error";
    }
}

vu_error_context_t *vu_get_last_error(void)
{
    return &tls_error_ctx;
}

void vu_clear_error(void)
{
    memset(&tls_error_ctx, 0, sizeof(tls_error_ctx));
}

void vu_set_error_internal(vu_error_t err, const char *file, int line,
                           const char *fmt, ...)
{
    tls_error_ctx.code = err;
    tls_error_ctx.file = file;
    tls_error_ctx.line = line;
    tls_error_ctx.pjsip_status = 0;

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(tls_error_ctx.message, sizeof(tls_error_ctx.message), fmt, args);
        va_end(args);
    } else {
        snprintf(tls_error_ctx.message, sizeof(tls_error_ctx.message),
                 "%s", vu_error_str(err));
    }
}

void vu_set_pjsip_error(vu_error_t err, int pjsip_status,
                        const char *file, int line, const char *fmt, ...)
{
    tls_error_ctx.code = err;
    tls_error_ctx.file = file;
    tls_error_ctx.line = line;
    tls_error_ctx.pjsip_status = pjsip_status;

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(tls_error_ctx.message, sizeof(tls_error_ctx.message),
                          fmt, args);
        va_end(args);

        /* Append PJSIP status if room */
        if (n > 0 && n < (int)(sizeof(tls_error_ctx.message) - 32)) {
            snprintf(tls_error_ctx.message + n,
                     sizeof(tls_error_ctx.message) - n,
                     " (pjsip=%d)", pjsip_status);
        }
    } else {
        snprintf(tls_error_ctx.message, sizeof(tls_error_ctx.message),
                 "%s (pjsip=%d)", vu_error_str(err), pjsip_status);
    }
}
