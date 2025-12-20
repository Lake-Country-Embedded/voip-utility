/*
 * voip-utility - SIP VoIP Testing Utility
 * Beep/tone detector implementation (stub)
 */

#include "audio/beep_detector.h"
#include <stdlib.h>
#include <string.h>

struct vu_beep_detector {
    vu_beep_config_t config;
    uint32_t sample_rate;
    vu_beep_callback_t callback;
    void *user_data;
    vu_beep_result_t result;
    /* State tracking */
    bool in_beep;
    double beep_start_time;
    double beep_sum_freq;
    double beep_sum_level;
    int beep_sample_count;
};

vu_beep_detector_t *vu_beep_detector_create(const vu_beep_config_t *config,
                                             uint32_t sample_rate)
{
    if (!config) return NULL;

    vu_beep_detector_t *detector = calloc(1, sizeof(vu_beep_detector_t));
    if (!detector) return NULL;

    detector->config = *config;
    detector->sample_rate = sample_rate;
    detector->in_beep = false;

    return detector;
}

void vu_beep_detector_destroy(vu_beep_detector_t *detector)
{
    if (!detector) return;
    if (detector->result.beeps) {
        free(detector->result.beeps);
    }
    free(detector);
}

void vu_beep_detector_set_callback(vu_beep_detector_t *detector,
                                    vu_beep_callback_t callback, void *user_data)
{
    if (!detector) return;
    detector->callback = callback;
    detector->user_data = user_data;
}

bool vu_beep_detector_process(vu_beep_detector_t *detector,
                               const vu_freq_result_t *freq_result,
                               const vu_level_result_t *level_result,
                               double current_time_sec,
                               vu_beep_event_t *out_event)
{
    if (!detector || !freq_result || !level_result) return false;

    bool is_tone = freq_result->valid && !level_result->is_silence;
    bool target_freq = true;

    /* Check if frequency matches target (if configured) */
    if (detector->config.target_freq_hz > 0) {
        float diff = freq_result->frequency - detector->config.target_freq_hz;
        if (diff < 0) diff = -diff;
        target_freq = (diff <= detector->config.freq_tolerance_hz);
    }

    is_tone = is_tone && target_freq && (freq_result->magnitude_db > detector->config.min_level_db);

    if (is_tone && !detector->in_beep) {
        /* Start of beep */
        detector->in_beep = true;
        detector->beep_start_time = current_time_sec;
        detector->beep_sum_freq = freq_result->frequency;
        detector->beep_sum_level = freq_result->magnitude_db;
        detector->beep_sample_count = 1;
    } else if (is_tone && detector->in_beep) {
        /* Continuing beep */
        detector->beep_sum_freq += freq_result->frequency;
        detector->beep_sum_level += freq_result->magnitude_db;
        detector->beep_sample_count++;
    } else if (!is_tone && detector->in_beep) {
        /* End of beep */
        detector->in_beep = false;
        double duration = current_time_sec - detector->beep_start_time;

        /* Check duration requirements */
        if (duration >= detector->config.min_duration_sec &&
            duration <= detector->config.max_duration_sec) {

            vu_beep_event_t event = {
                .start_time_sec = detector->beep_start_time,
                .end_time_sec = current_time_sec,
                .duration_sec = duration,
                .frequency_hz = detector->beep_sum_freq / detector->beep_sample_count,
                .avg_level_db = detector->beep_sum_level / detector->beep_sample_count,
                .peak_level_db = detector->beep_sum_level / detector->beep_sample_count,
                .beep_index = detector->result.beep_count
            };

            /* Store result */
            if (detector->result.beep_count >= detector->result.beep_capacity) {
                int new_cap = detector->result.beep_capacity == 0 ? 16 : detector->result.beep_capacity * 2;
                vu_beep_event_t *new_beeps = realloc(detector->result.beeps, new_cap * sizeof(vu_beep_event_t));
                if (new_beeps) {
                    detector->result.beeps = new_beeps;
                    detector->result.beep_capacity = new_cap;
                }
            }

            if (detector->result.beep_count < detector->result.beep_capacity) {
                detector->result.beeps[detector->result.beep_count++] = event;
                detector->result.total_beep_duration_sec += duration;
                detector->result.valid_beep_count++;

                if (detector->result.first_beep_time_sec == 0) {
                    detector->result.first_beep_time_sec = event.start_time_sec;
                }

                if (detector->callback) {
                    detector->callback(detector->user_data, &event);
                }

                if (out_event) {
                    *out_event = event;
                }

                return true;
            }
        }
    }

    return false;
}

const vu_beep_result_t *vu_beep_detector_get_result(const vu_beep_detector_t *detector)
{
    if (!detector) return NULL;
    return &detector->result;
}

void vu_beep_detector_reset(vu_beep_detector_t *detector)
{
    if (!detector) return;
    detector->in_beep = false;
    detector->result.beep_count = 0;
    detector->result.first_beep_time_sec = 0;
    detector->result.total_beep_duration_sec = 0;
    detector->result.valid_beep_count = 0;
}
