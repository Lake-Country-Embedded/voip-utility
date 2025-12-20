/*
 * voip-utility - SIP VoIP Testing Utility
 * Analyze command implementation
 */

#include "cli/cli.h"
#include "audio/analyzer.h"
#include "audio/beep_detector.h"
#include "util/log.h"
#include <stdio.h>

int vu_cmd_analyze(const vu_cli_args_t *args, vu_config_t *config)
{
    if (!args) return 1;

    const vu_analyze_opts_t *opts = &args->cmd.analyze;

    if (!opts->input_file) {
        VU_LOG_ERROR("Input file is required. Usage: voip-utility analyze <file>");
        return 1;
    }

    VU_LOG_INFO("Analyzing audio file: %s", opts->input_file);

    /* Analyze the file for frequencies */
    vu_analyzer_config_t analyzer_config = vu_analyzer_default_config();
    /* Use config values if available */
    if (config) {
        analyzer_config.min_level_db = config->beep.min_level_db;
        analyzer_config.freq_tolerance_hz = config->beep.freq_tolerance_hz;
    }

    size_t result_count = 0;
    vu_freq_result_t *results = vu_analyzer_analyze_file(opts->input_file, &analyzer_config, &result_count);

    if (!results) {
        VU_LOG_ERROR("Failed to analyze file: %s", opts->input_file);
        return 1;
    }

    VU_LOG_INFO("Analyzed %zu frames", result_count);

    /* Show frequency statistics */
    if (opts->show_stats || (!opts->show_beeps && !opts->show_dtmf)) {
        /* Find dominant frequencies */
        float freq_sum = 0;
        float level_sum = 0;
        float max_level = -200;
        int valid_count = 0;

        for (size_t i = 0; i < result_count; i++) {
            if (results[i].magnitude_db > max_level) {
                max_level = results[i].magnitude_db;
            }
            if (results[i].valid) {
                freq_sum += results[i].frequency;
                level_sum += results[i].magnitude_db;
                valid_count++;
            }
        }

        VU_LOG_INFO("Audio statistics:");
        VU_LOG_INFO("  Total frames: %zu", result_count);
        VU_LOG_INFO("  Valid frames (above threshold): %d", valid_count);
        VU_LOG_INFO("  Peak level: %.1f dB", max_level);
        VU_LOG_INFO("  Threshold: %.1f dB", analyzer_config.min_level_db);

        /* Show first few frame details for debugging */
        VU_LOG_DEBUG("First 5 frames:");
        for (size_t i = 0; i < result_count && i < 5; i++) {
            VU_LOG_DEBUG("  Frame %zu: freq=%.1f Hz, level=%.1f dB, valid=%d",
                        i, results[i].frequency, results[i].magnitude_db, results[i].valid);
        }
        if (valid_count > 0) {
            VU_LOG_INFO("  Average frequency: %.1f Hz", freq_sum / valid_count);
            VU_LOG_INFO("  Average level: %.1f dB", level_sum / valid_count);
        }
    }

    /* Detect beeps */
    if (opts->show_beeps) {
        vu_beep_config_t beep_config = config ? config->beep : (vu_beep_config_t){
            .min_level_db = -40,
            .min_duration_sec = 0.05,
            .max_duration_sec = 2.0,
            .target_freq_hz = 0,
            .freq_tolerance_hz = 50,
            .gap_duration_sec = 0.1
        };

        /* Use sample rate from file (assume 8000 if not available) */
        vu_beep_detector_t *detector = vu_beep_detector_create(&beep_config, 8000);
        if (detector) {
            double frame_duration = (double)analyzer_config.fft_size / 2 / 8000.0;  /* 50% overlap */
            vu_level_result_t level = {0};

            for (size_t i = 0; i < result_count; i++) {
                double time = i * frame_duration;
                level.rms_db = results[i].magnitude_db;
                level.is_silence = !results[i].valid;

                vu_beep_event_t event;
                if (vu_beep_detector_process(detector, &results[i], &level, time, &event)) {
                    VU_LOG_INFO("  Beep #%d: %.3fs - %.3fs (%.0fms) @ %.0fHz, %.1fdB",
                                event.beep_index + 1,
                                event.start_time_sec,
                                event.end_time_sec,
                                event.duration_sec * 1000,
                                event.frequency_hz,
                                event.avg_level_db);
                }
            }

            const vu_beep_result_t *result = vu_beep_detector_get_result(detector);
            VU_LOG_INFO("Detected beeps: %d", result->valid_beep_count);

            vu_beep_detector_destroy(detector);
        }
    }

    if (opts->show_dtmf) {
        VU_LOG_INFO("DTMF detection: (not yet implemented)");
    }

    vu_analyzer_free_results(results);
    return 0;
}
