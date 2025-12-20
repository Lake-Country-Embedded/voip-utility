/*
 * voip-utility - SIP VoIP Testing Utility
 * Media port integration
 */

#ifndef VU_MEDIA_H
#define VU_MEDIA_H

#include "util/error.h"
#include "core/call.h"
#include <pjsua-lib/pjsua.h>

/*
 * Connect audio analysis port to call
 * Allows intercepting audio for analysis/recording
 */
vu_error_t vu_media_connect_analysis(vu_call_t *call);

/*
 * Disconnect audio analysis from call
 */
void vu_media_disconnect_analysis(vu_call_t *call);

/*
 * Start recording call audio to WAV file
 */
vu_error_t vu_media_start_recording(vu_call_t *call, const char *path);

/*
 * Stop recording
 */
void vu_media_stop_recording(vu_call_t *call);

/*
 * Play audio file to call
 * Returns player ID on success, -1 on failure.
 */
int vu_media_play_file(vu_call_t *call, const char *path, bool loop);

/*
 * Stop audio playback
 */
void vu_media_stop_playback(vu_call_t *call, int player_id);

/*
 * Check if call has active media
 */
bool vu_media_is_active(const vu_call_t *call);

#endif /* VU_MEDIA_H */
