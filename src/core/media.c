/*
 * voip-utility - SIP VoIP Testing Utility
 * Media port integration - recording and playback
 */

#include "core/media.h"
#include "util/log.h"
#include "util/error.h"
#include <string.h>
#include <stdlib.h>

/* Player info stored in call */
typedef struct {
    pjsua_player_id id;
    pjsua_conf_port_id port;
} player_info_t;

/* Recorder info stored in call */
typedef struct {
    pjsua_recorder_id id;
    pjsua_conf_port_id port;
} recorder_info_t;

vu_error_t vu_media_connect_analysis(vu_call_t *call)
{
    if (!call) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "call is NULL");
        return VU_ERR_INVALID_ARG;
    }

    /* TODO: Create and connect custom audio analysis port */
    VU_LOG_DEBUG("Media analysis connected for call %d", call->pjsua_id);
    return VU_OK;
}

void vu_media_disconnect_analysis(vu_call_t *call)
{
    if (!call) return;

    /* TODO: Disconnect and destroy analysis port */
    VU_LOG_DEBUG("Media analysis disconnected for call %d", call->pjsua_id);
}

vu_error_t vu_media_start_recording(vu_call_t *call, const char *path)
{
    if (!call || !path) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Invalid arguments");
        return VU_ERR_INVALID_ARG;
    }

    if (call->pjsua_id == PJSUA_INVALID_ID) {
        VU_SET_ERROR(VU_ERR_CALL_NOT_ACTIVE, "Call not active");
        return VU_ERR_CALL_NOT_ACTIVE;
    }

    /* Check if already recording */
    if (call->recorder) {
        VU_LOG_WARN("Already recording call %d", call->pjsua_id);
        return VU_OK;
    }

    /* Create WAV recorder */
    pjsua_recorder_id rec_id;
    pj_str_t filename = pj_str((char *)path);

    pj_status_t status = pjsua_recorder_create(&filename, 0, NULL, 0, 0, &rec_id);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_MEDIA_ERROR, status, "Failed to create recorder");
        return VU_ERR_MEDIA_ERROR;
    }

    /* Get recorder's conference port */
    pjsua_conf_port_id rec_port = pjsua_recorder_get_conf_port(rec_id);

    /* Connect call's audio to recorder */
    pjsua_call_info ci;
    status = pjsua_call_get_info(call->pjsua_id, &ci);
    if (status != PJ_SUCCESS || ci.conf_slot == PJSUA_INVALID_ID) {
        pjsua_recorder_destroy(rec_id);
        VU_SET_ERROR(VU_ERR_MEDIA_ERROR, "Call has no active media");
        return VU_ERR_MEDIA_ERROR;
    }

    /* Connect call's receive audio to recorder (what we hear from remote) */
    status = pjsua_conf_connect(ci.conf_slot, rec_port);
    if (status != PJ_SUCCESS) {
        pjsua_recorder_destroy(rec_id);
        VU_SET_PJSIP_ERROR(VU_ERR_MEDIA_ERROR, status, "Failed to connect recorder");
        return VU_ERR_MEDIA_ERROR;
    }

    /* Store recorder info */
    recorder_info_t *rec_info = malloc(sizeof(recorder_info_t));
    if (!rec_info) {
        pjsua_conf_disconnect(ci.conf_slot, rec_port);
        pjsua_recorder_destroy(rec_id);
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Failed to allocate recorder info");
        return VU_ERR_NO_MEMORY;
    }
    rec_info->id = rec_id;
    rec_info->port = rec_port;
    call->recorder = rec_info;

    VU_LOG_INFO("Started recording call %d to %s", call->pjsua_id, path);
    return VU_OK;
}

void vu_media_stop_recording(vu_call_t *call)
{
    if (!call || !call->recorder) return;

    recorder_info_t *rec_info = (recorder_info_t *)call->recorder;

    /* Disconnect and destroy recorder */
    if (call->pjsua_id != PJSUA_INVALID_ID) {
        pjsua_call_info ci;
        if (pjsua_call_get_info(call->pjsua_id, &ci) == PJ_SUCCESS &&
            ci.conf_slot != PJSUA_INVALID_ID && ci.conf_slot >= 0) {
            pjsua_conf_disconnect(ci.conf_slot, rec_info->port);
        }
    }

    pjsua_recorder_destroy(rec_info->id);
    free(rec_info);
    call->recorder = NULL;

    VU_LOG_DEBUG("Stopped recording for call %d", call->pjsua_id);
}

int vu_media_play_file(vu_call_t *call, const char *path, bool loop)
{
    if (!call || !path) {
        VU_SET_ERROR(VU_ERR_INVALID_ARG, "Invalid arguments");
        return -1;
    }

    if (call->pjsua_id == PJSUA_INVALID_ID) {
        VU_SET_ERROR(VU_ERR_CALL_NOT_ACTIVE, "Call not active");
        return -1;
    }

    /* Create WAV player */
    pjsua_player_id player_id;
    pj_str_t filename = pj_str((char *)path);
    unsigned options = loop ? 0 : PJMEDIA_FILE_NO_LOOP;

    pj_status_t status = pjsua_player_create(&filename, options, &player_id);
    if (status != PJ_SUCCESS) {
        VU_SET_PJSIP_ERROR(VU_ERR_MEDIA_ERROR, status, "Failed to create player for %s", path);
        return -1;
    }

    /* Get player's conference port */
    pjsua_conf_port_id player_port = pjsua_player_get_conf_port(player_id);

    /* Get call's conference slot */
    pjsua_call_info ci;
    status = pjsua_call_get_info(call->pjsua_id, &ci);
    if (status != PJ_SUCCESS || ci.conf_slot == PJSUA_INVALID_ID) {
        pjsua_player_destroy(player_id);
        VU_SET_ERROR(VU_ERR_MEDIA_ERROR, "Call has no active media");
        return -1;
    }

    /* Connect player to call (remote will hear the audio) */
    status = pjsua_conf_connect(player_port, ci.conf_slot);
    if (status != PJ_SUCCESS) {
        pjsua_player_destroy(player_id);
        VU_SET_PJSIP_ERROR(VU_ERR_MEDIA_ERROR, status, "Failed to connect player");
        return -1;
    }

    /* Store player info */
    player_info_t *player_info = malloc(sizeof(player_info_t));
    if (!player_info) {
        pjsua_conf_disconnect(player_port, ci.conf_slot);
        pjsua_player_destroy(player_id);
        VU_SET_ERROR(VU_ERR_NO_MEMORY, "Failed to allocate player info");
        return -1;
    }
    player_info->id = player_id;
    player_info->port = player_port;
    call->player = player_info;

    VU_LOG_INFO("Playing file %s to call %d (loop=%d)", path, call->pjsua_id, loop);
    return player_id;
}

void vu_media_stop_playback(vu_call_t *call, int player_id)
{
    if (!call || !call->player) return;
    (void)player_id;  /* We only support one player per call for now */

    player_info_t *player_info = (player_info_t *)call->player;

    /* Disconnect and destroy player */
    if (call->pjsua_id != PJSUA_INVALID_ID) {
        pjsua_call_info ci;
        if (pjsua_call_get_info(call->pjsua_id, &ci) == PJ_SUCCESS &&
            ci.conf_slot != PJSUA_INVALID_ID && ci.conf_slot >= 0) {
            pjsua_conf_disconnect(player_info->port, ci.conf_slot);
        }
    }

    pjsua_player_destroy(player_info->id);
    free(player_info);
    call->player = NULL;

    VU_LOG_DEBUG("Stopped playback for call %d", call->pjsua_id);
}

bool vu_media_is_active(const vu_call_t *call)
{
    if (!call) return false;
    return call->media_state == VU_CALL_MEDIA_ACTIVE;
}
