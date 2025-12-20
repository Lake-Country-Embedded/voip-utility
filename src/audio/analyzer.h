/*
 * voip-utility - SIP VoIP Testing Utility
 * FFT-based audio analyzer for frequency detection
 */

#ifndef VU_ANALYZER_H
#define VU_ANALYZER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Analyzer configuration */
typedef struct vu_analyzer_config {
    int sample_rate;          /* Audio sample rate (e.g., 8000, 16000) */
    int fft_size;             /* FFT window size (power of 2, e.g., 512, 1024) */
    float min_level_db;       /* Minimum level to consider as signal (e.g., -40) */
    float freq_tolerance_hz;  /* Frequency detection tolerance (e.g., 50 Hz) */
} vu_analyzer_config_t;

/* Frequency detection result */
typedef struct vu_freq_result {
    float frequency;          /* Detected dominant frequency in Hz */
    float magnitude_db;       /* Magnitude in dB */
    bool valid;               /* True if valid frequency detected above threshold */
} vu_freq_result_t;

/* Audio level result */
typedef struct vu_level_result {
    float rms_db;             /* RMS level in dB */
    float peak_db;            /* Peak level in dB */
    bool is_silence;          /* True if below threshold */
} vu_level_result_t;

/* Opaque analyzer context */
typedef struct vu_analyzer vu_analyzer_t;

/*
 * Create audio analyzer
 * Returns NULL on failure.
 */
vu_analyzer_t *vu_analyzer_create(const vu_analyzer_config_t *config);

/*
 * Destroy analyzer and free resources
 */
void vu_analyzer_destroy(vu_analyzer_t *analyzer);

/*
 * Get default configuration
 */
vu_analyzer_config_t vu_analyzer_default_config(void);

/*
 * Analyze audio frame for dominant frequency
 * samples: pointer to int16_t PCM samples
 * count: number of samples
 * result: output frequency result
 * Returns true on success.
 */
bool vu_analyzer_detect_frequency(vu_analyzer_t *analyzer,
                                   const int16_t *samples, size_t count,
                                   vu_freq_result_t *result);

/*
 * Calculate audio level (RMS and peak)
 */
bool vu_analyzer_calculate_level(vu_analyzer_t *analyzer,
                                  const int16_t *samples, size_t count,
                                  vu_level_result_t *result);

/*
 * Check if frequency matches target within tolerance
 */
bool vu_analyzer_freq_matches(const vu_analyzer_t *analyzer,
                               float detected, float target);

/*
 * Analyze WAV file for frequencies
 * Returns detected frequencies at each frame interval.
 * Caller must free the returned array.
 */
vu_freq_result_t *vu_analyzer_analyze_file(const char *path,
                                            const vu_analyzer_config_t *config,
                                            size_t *count);

/*
 * Free results from analyze_file
 */
void vu_analyzer_free_results(vu_freq_result_t *results);

#endif /* VU_ANALYZER_H */
