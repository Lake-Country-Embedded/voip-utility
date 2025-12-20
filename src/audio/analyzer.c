/*
 * voip-utility - SIP VoIP Testing Utility
 * FFT-based audio analyzer implementation
 */

#include "audio/analyzer.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fftw3.h>

#define SILENCE_THRESHOLD_DB -60.0f

struct vu_analyzer {
    vu_analyzer_config_t config;
    float *window;             /* Hann window coefficients */
    float *input_buffer;       /* FFT input buffer */
    fftwf_complex *output;     /* FFT output */
    fftwf_plan plan;           /* FFTW plan */
};

/* Create Hann window */
static void create_hann_window(float *window, int size)
{
    for (int i = 0; i < size; i++) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (size - 1)));
    }
}

vu_analyzer_config_t vu_analyzer_default_config(void)
{
    vu_analyzer_config_t config = {
        .sample_rate = 8000,
        .fft_size = 512,
        .min_level_db = -40.0f,
        .freq_tolerance_hz = 50.0f
    };
    return config;
}

vu_analyzer_t *vu_analyzer_create(const vu_analyzer_config_t *config)
{
    if (!config) return NULL;
    
    /* FFT size must be power of 2 */
    if ((config->fft_size & (config->fft_size - 1)) != 0) {
        return NULL;
    }

    vu_analyzer_t *analyzer = calloc(1, sizeof(vu_analyzer_t));
    if (!analyzer) return NULL;

    analyzer->config = *config;

    /* Allocate buffers */
    analyzer->window = fftwf_alloc_real(config->fft_size);
    analyzer->input_buffer = fftwf_alloc_real(config->fft_size);
    analyzer->output = fftwf_alloc_complex(config->fft_size / 2 + 1);

    if (!analyzer->window || !analyzer->input_buffer || !analyzer->output) {
        vu_analyzer_destroy(analyzer);
        return NULL;
    }

    /* Create FFTW plan */
    analyzer->plan = fftwf_plan_dft_r2c_1d(config->fft_size,
                                            analyzer->input_buffer,
                                            analyzer->output,
                                            FFTW_ESTIMATE);
    if (!analyzer->plan) {
        vu_analyzer_destroy(analyzer);
        return NULL;
    }

    /* Initialize Hann window */
    create_hann_window(analyzer->window, config->fft_size);

    return analyzer;
}

void vu_analyzer_destroy(vu_analyzer_t *analyzer)
{
    if (!analyzer) return;

    if (analyzer->plan) {
        fftwf_destroy_plan(analyzer->plan);
    }
    if (analyzer->window) fftwf_free(analyzer->window);
    if (analyzer->input_buffer) fftwf_free(analyzer->input_buffer);
    if (analyzer->output) fftwf_free(analyzer->output);

    free(analyzer);
}

bool vu_analyzer_detect_frequency(vu_analyzer_t *analyzer,
                                   const int16_t *samples, size_t count,
                                   vu_freq_result_t *result)
{
    if (!analyzer || !samples || !result) return false;

    memset(result, 0, sizeof(*result));

    int fft_size = analyzer->config.fft_size;
    size_t samples_to_use = (count < (size_t)fft_size) ? count : (size_t)fft_size;

    /* Apply window and convert to float */
    for (size_t i = 0; i < samples_to_use; i++) {
        analyzer->input_buffer[i] = (samples[i] / 32768.0f) * analyzer->window[i];
    }
    /* Zero-pad if necessary */
    for (size_t i = samples_to_use; i < (size_t)fft_size; i++) {
        analyzer->input_buffer[i] = 0.0f;
    }

    /* Execute FFT */
    fftwf_execute(analyzer->plan);

    /* Find peak magnitude and frequency */
    float max_magnitude = 0.0f;
    int max_bin = 0;
    int num_bins = fft_size / 2;

    for (int i = 1; i < num_bins; i++) {
        float real = analyzer->output[i][0];
        float imag = analyzer->output[i][1];
        float magnitude = sqrtf(real * real + imag * imag);

        if (magnitude > max_magnitude) {
            max_magnitude = magnitude;
            max_bin = i;
        }
    }

    /* Convert to dB */
    float magnitude_db = 20.0f * log10f(max_magnitude / (fft_size / 2) + 1e-10f);

    /* Calculate frequency from bin index */
    float bin_width = (float)analyzer->config.sample_rate / fft_size;
    float frequency = max_bin * bin_width;

    result->frequency = frequency;
    result->magnitude_db = magnitude_db;
    result->valid = (magnitude_db > analyzer->config.min_level_db);

    return true;
}

bool vu_analyzer_calculate_level(vu_analyzer_t *analyzer,
                                  const int16_t *samples, size_t count,
                                  vu_level_result_t *result)
{
    if (!analyzer || !samples || !result || count == 0) return false;

    float sum_squares = 0.0f;
    int16_t peak = 0;

    for (size_t i = 0; i < count; i++) {
        int16_t sample = samples[i];
        float normalized = sample / 32768.0f;
        sum_squares += normalized * normalized;

        int16_t abs_sample = (sample < 0) ? -sample : sample;
        if (abs_sample > peak) peak = abs_sample;
    }

    float rms = sqrtf(sum_squares / count);
    float peak_normalized = peak / 32768.0f;

    result->rms_db = 20.0f * log10f(rms + 1e-10f);
    result->peak_db = 20.0f * log10f(peak_normalized + 1e-10f);
    result->is_silence = (result->rms_db < SILENCE_THRESHOLD_DB);

    return true;
}

bool vu_analyzer_freq_matches(const vu_analyzer_t *analyzer,
                               float detected, float target)
{
    if (!analyzer) return false;
    float diff = fabsf(detected - target);
    return diff <= analyzer->config.freq_tolerance_hz;
}

/* WAV chunk header */
typedef struct {
    char id[4];
    uint32_t size;
} wav_chunk_t;

/* WAV format chunk data */
typedef struct {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} wav_fmt_t;

vu_freq_result_t *vu_analyzer_analyze_file(const char *path,
                                            const vu_analyzer_config_t *config,
                                            size_t *count)
{
    if (!path || !count) return NULL;

    *count = 0;

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    /* Read RIFF header */
    wav_chunk_t riff;
    char wave[4];
    if (fread(&riff, sizeof(riff), 1, f) != 1 ||
        fread(wave, 4, 1, f) != 1) {
        fclose(f);
        return NULL;
    }

    /* Validate RIFF/WAVE */
    if (memcmp(riff.id, "RIFF", 4) != 0 || memcmp(wave, "WAVE", 4) != 0) {
        fclose(f);
        return NULL;
    }

    /* Scan for fmt and data chunks */
    wav_fmt_t fmt = {0};
    uint32_t data_size = 0;
    long data_offset = 0;
    bool found_fmt = false, found_data = false;

    while (!found_fmt || !found_data) {
        wav_chunk_t chunk;
        if (fread(&chunk, sizeof(chunk), 1, f) != 1) break;

        if (memcmp(chunk.id, "fmt ", 4) == 0) {
            if (fread(&fmt, sizeof(fmt), 1, f) != 1) break;
            /* Skip any extra fmt bytes */
            if (chunk.size > sizeof(fmt)) {
                fseek(f, chunk.size - sizeof(fmt), SEEK_CUR);
            }
            found_fmt = true;
        } else if (memcmp(chunk.id, "data", 4) == 0) {
            data_size = chunk.size;
            data_offset = ftell(f);
            found_data = true;
        } else {
            /* Skip unknown chunk */
            fseek(f, chunk.size, SEEK_CUR);
        }
    }

    if (!found_fmt || !found_data || fmt.bits_per_sample == 0) {
        fclose(f);
        return NULL;
    }

    /* Create analyzer with file's sample rate or config */
    vu_analyzer_config_t file_config = config ? *config : vu_analyzer_default_config();
    file_config.sample_rate = fmt.sample_rate;

    vu_analyzer_t *analyzer = vu_analyzer_create(&file_config);
    if (!analyzer) {
        fclose(f);
        return NULL;
    }

    /* Calculate number of frames */
    int frame_size = file_config.fft_size;
    int hop_size = frame_size / 2;  /* 50% overlap */
    int num_samples = data_size / (fmt.bits_per_sample / 8) / fmt.num_channels;
    int num_frames = (num_samples - frame_size) / hop_size + 1;

    if (num_frames <= 0) {
        vu_analyzer_destroy(analyzer);
        fclose(f);
        return NULL;
    }

    vu_freq_result_t *results = calloc(num_frames, sizeof(vu_freq_result_t));
    if (!results) {
        vu_analyzer_destroy(analyzer);
        fclose(f);
        return NULL;
    }

    /* Read and analyze each frame */
    int16_t *samples = malloc(frame_size * sizeof(int16_t));
    if (!samples) {
        free(results);
        vu_analyzer_destroy(analyzer);
        fclose(f);
        return NULL;
    }

    int result_count = 0;
    for (int frame = 0; frame < num_frames && result_count < num_frames; frame++) {
        long file_offset = data_offset + frame * hop_size * sizeof(int16_t);
        fseek(f, file_offset, SEEK_SET);

        size_t samples_read = fread(samples, sizeof(int16_t), frame_size, f);
        if (samples_read < (size_t)frame_size / 2) break;

        if (vu_analyzer_detect_frequency(analyzer, samples, samples_read, &results[result_count])) {
            result_count++;
        }
    }

    free(samples);
    vu_analyzer_destroy(analyzer);
    fclose(f);

    *count = result_count;
    return results;
}

void vu_analyzer_free_results(vu_freq_result_t *results)
{
    free(results);
}
