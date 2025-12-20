/*
 * voip-utility - SIP VoIP Testing Utility
 * DTMF handling implementation
 */

#include "core/dtmf.h"
#include "core/sip_ua.h"
#include "util/log.h"
#include "util/error.h"
#include <string.h>

const char *vu_dtmf_method_name(vu_dtmf_method_t method)
{
    switch (method) {
    case VU_DTMF_RFC2833:  return "rfc2833";
    case VU_DTMF_INBAND:   return "inband";
    case VU_DTMF_SIP_INFO: return "sip_info";
    default:               return "unknown";
    }
}

vu_dtmf_method_t vu_dtmf_method_from_string(const char *str)
{
    if (!str) return VU_DTMF_RFC2833;

    if (strcasecmp(str, "rfc2833") == 0 || strcasecmp(str, "rtp") == 0)
        return VU_DTMF_RFC2833;
    if (strcasecmp(str, "inband") == 0 || strcasecmp(str, "audio") == 0)
        return VU_DTMF_INBAND;
    if (strcasecmp(str, "sip_info") == 0 || strcasecmp(str, "info") == 0)
        return VU_DTMF_SIP_INFO;

    return VU_DTMF_RFC2833;
}

vu_dtmf_opts_t vu_dtmf_default_opts(void)
{
    vu_dtmf_opts_t opts = {
        .method = VU_DTMF_RFC2833,
        .duration_ms = 100,
        .gap_ms = 100
    };
    return opts;
}

vu_error_t vu_dtmf_send(vu_call_t *call, const char *digits, const vu_dtmf_opts_t *opts)
{
    if (!call || !digits) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Invalid arguments");
        return VU_ERR_INVALID_ARG;
    }

    if (call->pjsua_id == PJSUA_INVALID_ID) {
        VU_SET_ERROR(VU_ERR_CALL_NOT_ACTIVE, "Call not active");
        return VU_ERR_CALL_NOT_ACTIVE;
    }

    vu_dtmf_opts_t opt = opts ? *opts : vu_dtmf_default_opts();

    VU_LOG_INFO("Sending DTMF '%s' on call %d (method=%s)",
                digits, call->pjsua_id, vu_dtmf_method_name(opt.method));

    pjsua_call_send_dtmf_param param;
    pjsua_call_send_dtmf_param_default(&param);

    param.digits = pj_str((char *)digits);
    param.duration = opt.duration_ms;

    switch (opt.method) {
    case VU_DTMF_RFC2833:
        param.method = PJSUA_DTMF_METHOD_RFC2833;
        break;
    case VU_DTMF_SIP_INFO:
        param.method = PJSUA_DTMF_METHOD_SIP_INFO;
        break;
    case VU_DTMF_INBAND:
        /* In-band requires tone generator - not supported yet */
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "In-band DTMF not yet supported");
        return VU_ERR_INVALID_ARG;
    }

    pj_status_t status = pjsua_call_send_dtmf(call->pjsua_id, &param);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_CALL_FAILED, status, "Failed to send DTMF");
        return VU_ERR_CALL_FAILED;
    }

    return VU_OK;
}

vu_error_t vu_dtmf_send_digit(vu_call_t *call, char digit, const vu_dtmf_opts_t *opts)
{
    char digits[2] = {digit, '\0'};
    return vu_dtmf_send(call, digits, opts);
}
