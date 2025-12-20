/*
 * voip-utility - SIP VoIP Testing Utility
 * Custom PJMEDIA audio port (stub implementation)
 */

#include "audio/audio_port.h"
#include "util/log.h"
#include <stdlib.h>
#include <string.h>

struct vu_audio_port {
    pjmedia_port base;
    pj_pool_t *pool;
    vu_analyzer_t *analyzer;
    vu_beep_detector_t *beep_detector;
    vu_recorder_t *recorder;
    uint32_t sample_rate;
};

static pj_status_t audio_port_put_frame(pjmedia_port *this_port, pjmedia_frame *frame);
static pj_status_t audio_port_get_frame(pjmedia_port *this_port, pjmedia_frame *frame);
static pj_status_t audio_port_on_destroy(pjmedia_port *this_port);

vu_audio_port_t *vu_audio_port_create(pj_pool_t *pool, uint32_t sample_rate)
{
    if (!pool) return NULL;

    vu_audio_port_t *port = pj_pool_zalloc(pool, sizeof(vu_audio_port_t));
    if (!port) return NULL;

    port->pool = pool;
    port->sample_rate = sample_rate;

    /* Initialize PJMEDIA port */
    pj_str_t name = pj_str("vu_audio_port");
    pj_status_t status = pjmedia_port_info_init(&port->base.info, &name,
                                                 0x12345678,  /* Signature */
                                                 sample_rate,
                                                 1,  /* Channels */
                                                 16, /* Bits */
                                                 sample_rate * 20 / 1000); /* 20ms frame */
    if (status != PJ_SUCCESS) {
        return NULL;
    }

    port->base.put_frame = audio_port_put_frame;
    port->base.get_frame = audio_port_get_frame;
    port->base.on_destroy = audio_port_on_destroy;

    VU_LOG_DEBUG("Created audio port: sample_rate=%u", sample_rate);
    return port;
}

void vu_audio_port_destroy(vu_audio_port_t *port)
{
    if (!port) return;
    /* Port memory is from pool, will be freed with pool */
}

pjmedia_port *vu_audio_port_get_pjmedia_port(vu_audio_port_t *port)
{
    return port ? &port->base : NULL;
}

void vu_audio_port_set_analyzer(vu_audio_port_t *port, vu_analyzer_t *analyzer)
{
    if (port) port->analyzer = analyzer;
}

void vu_audio_port_set_beep_detector(vu_audio_port_t *port, vu_beep_detector_t *detector)
{
    if (port) port->beep_detector = detector;
}

void vu_audio_port_set_recorder(vu_audio_port_t *port, vu_recorder_t *recorder)
{
    if (port) port->recorder = recorder;
}

static pj_status_t audio_port_put_frame(pjmedia_port *this_port, pjmedia_frame *frame)
{
    vu_audio_port_t *port = (vu_audio_port_t *)this_port;

    if (frame->type != PJMEDIA_FRAME_TYPE_AUDIO) {
        return PJ_SUCCESS;
    }

    int16_t *samples = (int16_t *)frame->buf;
    size_t sample_count = frame->size / sizeof(int16_t);

    /* Feed to analyzer */
    if (port->analyzer) {
        vu_audio_analysis_t analysis;
        vu_analyzer_process(port->analyzer, samples, sample_count, &analysis);

        /* Feed to beep detector */
        if (port->beep_detector) {
            vu_beep_event_t beep;
            vu_beep_detector_process(port->beep_detector, &analysis, &beep);
        }
    }

    /* Record if enabled */
    if (port->recorder) {
        vu_recorder_write(port->recorder, samples, sample_count);
    }

    return PJ_SUCCESS;
}

static pj_status_t audio_port_get_frame(pjmedia_port *this_port, pjmedia_frame *frame)
{
    (void)this_port;
    /* Output silence - we're receive-only */
    frame->type = PJMEDIA_FRAME_TYPE_NONE;
    return PJ_SUCCESS;
}

static pj_status_t audio_port_on_destroy(pjmedia_port *this_port)
{
    (void)this_port;
    return PJ_SUCCESS;
}
