/*
 * voip-utility - SIP VoIP Testing Utility
 * Call handling implementation
 */

#include "core/call.h"
#include "core/sip_ua.h"
#include "core/media.h"
#include "util/log.h"
#include "util/error.h"
#include "util/time_util.h"
#include <string.h>
#include <stdio.h>

const char *vu_call_state_name(vu_call_state_t state)
{
    switch (state) {
    case VU_CALL_STATE_NULL:         return "null";
    case VU_CALL_STATE_CALLING:      return "calling";
    case VU_CALL_STATE_INCOMING:     return "incoming";
    case VU_CALL_STATE_EARLY:        return "early";
    case VU_CALL_STATE_CONNECTING:   return "connecting";
    case VU_CALL_STATE_CONFIRMED:    return "confirmed";
    case VU_CALL_STATE_DISCONNECTED: return "disconnected";
    default:                          return "unknown";
    }
}

const char *vu_call_media_state_name(vu_call_media_state_t state)
{
    switch (state) {
    case VU_CALL_MEDIA_NONE:        return "none";
    case VU_CALL_MEDIA_ACTIVE:      return "active";
    case VU_CALL_MEDIA_LOCAL_HOLD:  return "local_hold";
    case VU_CALL_MEDIA_REMOTE_HOLD: return "remote_hold";
    case VU_CALL_MEDIA_ERROR:       return "error";
    default:                         return "unknown";
    }
}

void vu_call_manager_init(vu_call_manager_t *mgr)
{
    if (!mgr) return;

    memset(mgr, 0, sizeof(*mgr));
    for (int i = 0; i < VU_MAX_CALLS; i++) {
        mgr->calls[i].pjsua_id = PJSUA_INVALID_ID;
    }
}

void vu_call_manager_cleanup(vu_call_manager_t *mgr)
{
    if (!mgr) return;

    vu_call_hangup_all(mgr);
    memset(mgr, 0, sizeof(*mgr));
}

/* Find a free call slot */
static vu_call_t *find_free_slot(vu_call_manager_t *mgr)
{
    for (int i = 0; i < VU_MAX_CALLS; i++) {
        if (mgr->calls[i].pjsua_id == PJSUA_INVALID_ID &&
            mgr->calls[i].state == VU_CALL_STATE_NULL) {
            return &mgr->calls[i];
        }
    }
    return NULL;
}

vu_call_t *vu_call_make(vu_call_manager_t *mgr, vu_account_t *account,
                         const char *uri)
{
    if (!mgr || !account || !uri) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Invalid arguments");
        return NULL;
    }

    if (!vu_ua_is_running()) {
        VU_SET_ERROR(VU_ERR_NOT_INITIALIZED, "UA not initialized");
        return NULL;
    }

    if (account->pjsua_id == PJSUA_INVALID_ID) {
        VU_SET_ERROR(VU_ERR_NO_ACCOUNT, "Account not registered");
        return NULL;
    }

    vu_call_t *call = find_free_slot(mgr);
    if (!call) {
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "No free call slots");
        return NULL;
    }

    /* Initialize call */
    memset(call, 0, sizeof(*call));
    call->pjsua_id = PJSUA_INVALID_ID;
    call->direction = VU_CALL_DIR_OUTBOUND;
    strncpy(call->remote_uri, uri, sizeof(call->remote_uri) - 1);
    strncpy(call->account_id, account->config.id, sizeof(call->account_id) - 1);
    call->start_time_ms = vu_time_now_ms();

    /* Make call - add transport=udp parameter to force UDP transport selection */
    char uri_buf[512];
    if (strstr(uri, "transport=") == NULL) {
        snprintf(uri_buf, sizeof(uri_buf), "%s;transport=udp", uri);
    } else {
        strncpy(uri_buf, uri, sizeof(uri_buf) - 1);
        uri_buf[sizeof(uri_buf) - 1] = '\0';
    }

    pj_str_t dest_uri = pj_str(uri_buf);
    pj_status_t status = pjsua_call_make_call(account->pjsua_id, &dest_uri,
                                               NULL, NULL, NULL, &call->pjsua_id);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_CALL_FAILED, status, "Failed to make call to %s", uri);
        call->pjsua_id = PJSUA_INVALID_ID;
        return NULL;
    }

    call->state = VU_CALL_STATE_CALLING;
    mgr->call_count++;

    VU_LOG_INFO("Making call: id=%d to=%s from=%s",
                call->pjsua_id, uri_buf, account->config.id);

    return call;
}

vu_error_t vu_call_answer(vu_call_t *call, int code)
{
    if (!call) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "call is NULL");
        return VU_ERR_INVALID_ARG;
    }

    if (call->pjsua_id == PJSUA_INVALID_ID) {
        VU_SET_ERROR(VU_ERR_CALL_NOT_ACTIVE, "Call not active");
        return VU_ERR_CALL_NOT_ACTIVE;
    }

    pj_status_t status = pjsua_call_answer(call->pjsua_id, code, NULL, NULL);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_CALL_FAILED, status, "Failed to answer call");
        return VU_ERR_CALL_FAILED;
    }

    VU_LOG_INFO("Answered call: id=%d code=%d", call->pjsua_id, code);
    return VU_OK;
}

vu_error_t vu_call_hangup(vu_call_t *call, int code)
{
    if (!call) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "call is NULL");
        return VU_ERR_INVALID_ARG;
    }

    /* Capture call ID atomically to avoid race with callback */
    pjsua_call_id call_id = call->pjsua_id;
    if (call_id == PJSUA_INVALID_ID) {
        return VU_OK;  /* Already hung up */
    }

    /* Mark as invalid before calling PJSIP to prevent double-hangup */
    call->pjsua_id = PJSUA_INVALID_ID;

    pjsua_call_hangup(call_id, code, NULL, NULL);

    VU_LOG_INFO("Hung up call: id=%d code=%d", call_id, code);

    call->end_time_ms = vu_time_now_ms();
    call->state = VU_CALL_STATE_DISCONNECTED;

    return VU_OK;
}

void vu_call_hangup_all(vu_call_manager_t *mgr)
{
    if (!mgr) return;

    pjsua_call_hangup_all();

    for (int i = 0; i < VU_MAX_CALLS; i++) {
        if (mgr->calls[i].pjsua_id != PJSUA_INVALID_ID) {
            mgr->calls[i].end_time_ms = vu_time_now_ms();
            mgr->calls[i].state = VU_CALL_STATE_DISCONNECTED;
            mgr->calls[i].pjsua_id = PJSUA_INVALID_ID;
        }
    }
    mgr->call_count = 0;
}

vu_call_t *vu_call_find_by_pjsua_id(vu_call_manager_t *mgr, pjsua_call_id pjsua_id)
{
    if (!mgr || pjsua_id == PJSUA_INVALID_ID) return NULL;

    for (int i = 0; i < VU_MAX_CALLS; i++) {
        if (mgr->calls[i].pjsua_id == pjsua_id) {
            return &mgr->calls[i];
        }
    }
    return NULL;
}

vu_call_t *vu_call_find_active(vu_call_manager_t *mgr)
{
    if (!mgr) return NULL;

    for (int i = 0; i < VU_MAX_CALLS; i++) {
        if (mgr->calls[i].pjsua_id != PJSUA_INVALID_ID &&
            mgr->calls[i].state != VU_CALL_STATE_DISCONNECTED) {
            return &mgr->calls[i];
        }
    }
    return NULL;
}

vu_error_t vu_call_wait_state(vu_call_t *call, vu_call_state_t state, int timeout_sec)
{
    if (!call) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "call is NULL");
        return VU_ERR_INVALID_ARG;
    }

    vu_timer_t timer;
    vu_timer_start(&timer, timeout_sec * 1000);

    while (!vu_timer_expired(&timer)) {
        if (call->state == state) {
            return VU_OK;
        }
        if (call->state == VU_CALL_STATE_DISCONNECTED) {
            VU_SET_ERROR(VU_ERR_CALL_FAILED, "Call disconnected");
            return VU_ERR_CALL_FAILED;
        }

        vu_ua_poll(100);
    }

    VU_SET_ERROR(VU_ERR_TIMEOUT, "Timeout waiting for call state %s",
                 vu_call_state_name(state));
    return VU_ERR_TIMEOUT;
}

vu_error_t vu_call_wait_connected(vu_call_t *call, int timeout_sec)
{
    return vu_call_wait_state(call, VU_CALL_STATE_CONFIRMED, timeout_sec);
}

vu_call_t *vu_call_wait_incoming(vu_call_manager_t *mgr, int timeout_sec)
{
    if (!mgr) return NULL;

    vu_timer_t timer;
    vu_timer_start(&timer, timeout_sec * 1000);

    while (!vu_timer_expired(&timer)) {
        for (int i = 0; i < VU_MAX_CALLS; i++) {
            if (mgr->calls[i].state == VU_CALL_STATE_INCOMING) {
                return &mgr->calls[i];
            }
        }

        vu_ua_poll(100);
    }

    return NULL;
}

double vu_call_get_duration(const vu_call_t *call)
{
    if (!call) return 0;

    uint64_t end = call->end_time_ms;
    if (end == 0) {
        end = vu_time_now_ms();
    }

    uint64_t start = call->connect_time_ms;
    if (start == 0) {
        start = call->start_time_ms;
    }

    return (double)(end - start) / 1000.0;
}

void vu_call_on_state_change(vu_call_t *call, pjsua_call_info *ci)
{
    if (!call || !ci) return;

    call->last_status_code = ci->last_status;

    /* Convert PJSIP state to our state */
    switch (ci->state) {
    case PJSIP_INV_STATE_NULL:
        call->state = VU_CALL_STATE_NULL;
        break;
    case PJSIP_INV_STATE_CALLING:
        call->state = VU_CALL_STATE_CALLING;
        break;
    case PJSIP_INV_STATE_INCOMING:
        call->state = VU_CALL_STATE_INCOMING;
        break;
    case PJSIP_INV_STATE_EARLY:
        call->state = VU_CALL_STATE_EARLY;
        break;
    case PJSIP_INV_STATE_CONNECTING:
        call->state = VU_CALL_STATE_CONNECTING;
        break;
    case PJSIP_INV_STATE_CONFIRMED:
        call->state = VU_CALL_STATE_CONFIRMED;
        if (call->connect_time_ms == 0) {
            call->connect_time_ms = vu_time_now_ms();
        }
        break;
    case PJSIP_INV_STATE_DISCONNECTED:
        /* Cleanup media before marking call as disconnected */
        vu_media_stop_recording(call);
        vu_media_stop_playback(call, -1);
        call->state = VU_CALL_STATE_DISCONNECTED;
        call->end_time_ms = vu_time_now_ms();
        call->pjsua_id = PJSUA_INVALID_ID;
        break;
    }

    VU_LOG_DEBUG("Call %d state: %s", ci->id, vu_call_state_name(call->state));
}

void vu_call_on_media_state(vu_call_t *call, pjsua_call_info *ci)
{
    if (!call || !ci) return;

    switch (ci->media_status) {
    case PJSUA_CALL_MEDIA_NONE:
        call->media_state = VU_CALL_MEDIA_NONE;
        break;
    case PJSUA_CALL_MEDIA_ACTIVE:
        call->media_state = VU_CALL_MEDIA_ACTIVE;
        call->conf_port = ci->conf_slot;
        break;
    case PJSUA_CALL_MEDIA_LOCAL_HOLD:
        call->media_state = VU_CALL_MEDIA_LOCAL_HOLD;
        break;
    case PJSUA_CALL_MEDIA_REMOTE_HOLD:
        call->media_state = VU_CALL_MEDIA_REMOTE_HOLD;
        break;
    case PJSUA_CALL_MEDIA_ERROR:
        call->media_state = VU_CALL_MEDIA_ERROR;
        break;
    default:
        break;
    }

    VU_LOG_DEBUG("Call %d media: %s", ci->id, vu_call_media_state_name(call->media_state));
}

vu_call_t *vu_call_on_incoming(vu_call_manager_t *mgr, pjsua_call_id call_id,
                                pjsua_call_info *ci)
{
    if (!mgr || !ci) return NULL;

    vu_call_t *call = find_free_slot(mgr);
    if (!call) {
        VU_LOG_WARN("No free call slots for incoming call");
        pjsua_call_hangup(call_id, 486, NULL, NULL);  /* Busy Here */
        return NULL;
    }

    /* Initialize call */
    memset(call, 0, sizeof(*call));
    call->pjsua_id = call_id;
    call->direction = VU_CALL_DIR_INBOUND;
    call->state = VU_CALL_STATE_INCOMING;
    call->start_time_ms = vu_time_now_ms();

    /* Copy URIs */
    if (ci->remote_info.slen > 0) {
        size_t len = ci->remote_info.slen < sizeof(call->remote_uri) - 1
                     ? ci->remote_info.slen : sizeof(call->remote_uri) - 1;
        memcpy(call->remote_uri, ci->remote_info.ptr, len);
        call->remote_uri[len] = '\0';
    }
    if (ci->local_info.slen > 0) {
        size_t len = ci->local_info.slen < sizeof(call->local_uri) - 1
                     ? ci->local_info.slen : sizeof(call->local_uri) - 1;
        memcpy(call->local_uri, ci->local_info.ptr, len);
        call->local_uri[len] = '\0';
    }

    mgr->call_count++;

    VU_LOG_INFO("Incoming call: id=%d from=%s", call_id, call->remote_uri);
    return call;
}

void vu_call_on_dtmf_digit(vu_call_t *call, char digit, int duration_ms)
{
    if (!call) return;

    VU_LOG_INFO("Received DTMF digit '%c' (duration=%dms) on call %d",
                digit, duration_ms, call->pjsua_id);

    if (call->dtmf_count < VU_MAX_DTMF_DIGITS) {
        call->dtmf_buffer[call->dtmf_count++] = digit;
        call->dtmf_buffer[call->dtmf_count] = '\0';
    } else {
        VU_LOG_WARN("DTMF buffer full, discarding digit '%c'", digit);
    }
}

const char *vu_call_get_dtmf_digits(const vu_call_t *call)
{
    if (!call) return "";
    return call->dtmf_buffer;
}

void vu_call_clear_dtmf(vu_call_t *call)
{
    if (!call) return;
    call->dtmf_buffer[0] = '\0';
    call->dtmf_count = 0;
}

vu_error_t vu_call_wait_dtmf(vu_call_t *call, const char *pattern, int timeout_sec)
{
    if (!call || !pattern) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Invalid arguments");
        return VU_ERR_INVALID_ARG;
    }

    size_t pattern_len = strlen(pattern);
    if (pattern_len == 0) {
        return VU_OK;  /* Empty pattern matches immediately */
    }

    vu_timer_t timer;
    vu_timer_start(&timer, timeout_sec * 1000);

    while (!vu_timer_expired(&timer)) {
        /* Check if buffer contains pattern */
        if (call->dtmf_count >= (int)pattern_len) {
            const char *found = strstr(call->dtmf_buffer, pattern);
            if (found) {
                VU_LOG_INFO("DTMF pattern '%s' matched", pattern);
                return VU_OK;
            }
        }

        /* Check if call is still active */
        if (call->state == VU_CALL_STATE_DISCONNECTED) {
            VU_SET_ERROR(VU_ERR_CALL_NOT_ACTIVE, "Call disconnected while waiting for DTMF");
            return VU_ERR_CALL_NOT_ACTIVE;
        }

        vu_ua_poll(50);
    }

    VU_SET_ERROR(VU_ERR_TIMEOUT, "Timeout waiting for DTMF pattern '%s'", pattern);
    return VU_ERR_TIMEOUT;
}
