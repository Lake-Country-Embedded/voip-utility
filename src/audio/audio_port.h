/*
 * voip-utility - SIP VoIP Testing Utility
 * Custom PJMEDIA audio port for analysis
 */

#ifndef VU_AUDIO_PORT_H
#define VU_AUDIO_PORT_H

#include "audio/analyzer.h"
#include "audio/beep_detector.h"
#include "audio/recorder.h"
#include "util/error.h"
#include <pjmedia.h>

typedef struct vu_audio_port vu_audio_port_t;

vu_audio_port_t *vu_audio_port_create(pj_pool_t *pool, uint32_t sample_rate);
void vu_audio_port_destroy(vu_audio_port_t *port);

/* Get PJMEDIA port for connecting to conference bridge */
pjmedia_port *vu_audio_port_get_pjmedia_port(vu_audio_port_t *port);

/* Set analyzer for audio processing */
void vu_audio_port_set_analyzer(vu_audio_port_t *port, vu_analyzer_t *analyzer);

/* Set beep detector */
void vu_audio_port_set_beep_detector(vu_audio_port_t *port, vu_beep_detector_t *detector);

/* Set recorder for saving audio */
void vu_audio_port_set_recorder(vu_audio_port_t *port, vu_recorder_t *recorder);

#endif /* VU_AUDIO_PORT_H */
