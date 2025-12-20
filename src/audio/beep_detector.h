/*
 * voip-utility - SIP VoIP Testing Utility
 * Beep/tone detector
 */

#ifndef VU_BEEP_DETECTOR_H
#define VU_BEEP_DETECTOR_H

#include "audio/analyzer.h"
#include "config/config.h"
#include <stdbool.h>

/* Detected beep event */
typedef struct vu_beep_event {
    double start_time_sec;
    double end_time_sec;
    double duration_sec;
    double frequency_hz;
    double avg_level_db;
    double peak_level_db;
    int beep_index;
} vu_beep_event_t;

/* Detection results */
typedef struct vu_beep_result {
    vu_beep_event_t *beeps;
    int beep_count;
    int beep_capacity;
    double first_beep_time_sec;
    double total_beep_duration_sec;
    int valid_beep_count;
} vu_beep_result_t;

/* Callback for real-time beep detection */
typedef void (*vu_beep_callback_t)(void *user_data, const vu_beep_event_t *beep);

/* Opaque detector handle */
typedef struct vu_beep_detector vu_beep_detector_t;

/*
 * Create beep detector with configuration
 */
vu_beep_detector_t *vu_beep_detector_create(const vu_beep_config_t *config,
                                             uint32_t sample_rate);

/*
 * Destroy beep detector
 */
void vu_beep_detector_destroy(vu_beep_detector_t *detector);

/*
 * Set callback for real-time detection
 */
void vu_beep_detector_set_callback(vu_beep_detector_t *detector,
                                    vu_beep_callback_t callback, void *user_data);

/*
 * Process audio analysis frame
 * Returns true if a beep just ended (event available)
 */
bool vu_beep_detector_process(vu_beep_detector_t *detector,
                               const vu_freq_result_t *freq_result,
                               const vu_level_result_t *level_result,
                               double current_time_sec,
                               vu_beep_event_t *out_event);

/*
 * Get detection results
 */
const vu_beep_result_t *vu_beep_detector_get_result(const vu_beep_detector_t *detector);

/*
 * Reset detector state
 */
void vu_beep_detector_reset(vu_beep_detector_t *detector);

#endif /* VU_BEEP_DETECTOR_H */
