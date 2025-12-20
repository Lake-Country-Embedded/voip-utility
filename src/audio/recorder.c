/*
 * voip-utility - SIP VoIP Testing Utility
 * WAV recorder (stub implementation)
 */

#include "audio/recorder.h"
#include "util/log.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct vu_recorder {
    FILE *fp;
    uint32_t sample_rate;
    int channels;
    uint32_t samples_written;
    char path[512];
};

/* WAV header structure */
#pragma pack(push, 1)
typedef struct {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} wav_header_t;
#pragma pack(pop)

vu_recorder_t *vu_recorder_create(const char *path, uint32_t sample_rate, int channels)
{
    if (!path) return NULL;

    vu_recorder_t *rec = calloc(1, sizeof(vu_recorder_t));
    if (!rec) return NULL;

    rec->fp = fopen(path, "wb");
    if (!rec->fp) {
        VU_LOG_ERROR("Failed to open %s for writing", path);
        free(rec);
        return NULL;
    }

    rec->sample_rate = sample_rate;
    rec->channels = channels;
    strncpy(rec->path, path, sizeof(rec->path) - 1);

    /* Write placeholder header */
    wav_header_t header = {0};
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.fmt_size = 16;
    header.audio_format = 1;  /* PCM */
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;
    header.block_align = channels * 2;
    header.byte_rate = sample_rate * channels * 2;
    memcpy(header.data, "data", 4);

    fwrite(&header, sizeof(header), 1, rec->fp);

    VU_LOG_DEBUG("Created WAV recorder: %s", path);
    return rec;
}

void vu_recorder_destroy(vu_recorder_t *recorder)
{
    if (!recorder) return;

    if (recorder->fp) {
        /* Update header with final sizes */
        uint32_t data_size = recorder->samples_written * recorder->channels * 2;
        uint32_t file_size = data_size + sizeof(wav_header_t) - 8;

        fseek(recorder->fp, 4, SEEK_SET);
        fwrite(&file_size, 4, 1, recorder->fp);
        fseek(recorder->fp, 40, SEEK_SET);
        fwrite(&data_size, 4, 1, recorder->fp);

        fclose(recorder->fp);
        VU_LOG_INFO("Saved WAV: %s (%.2fs)", recorder->path,
                    (double)recorder->samples_written / recorder->sample_rate);
    }

    free(recorder);
}

vu_error_t vu_recorder_write(vu_recorder_t *recorder, const int16_t *samples, size_t count)
{
    if (!recorder || !samples) return VU_ERR_INVALID_ARG;
    if (!recorder->fp) return VU_ERR_IO;

    size_t written = fwrite(samples, sizeof(int16_t), count, recorder->fp);
    if (written != count) {
        return VU_ERR_IO;
    }

    recorder->samples_written += count / recorder->channels;
    return VU_OK;
}

double vu_recorder_get_duration(const vu_recorder_t *recorder)
{
    if (!recorder) return 0;
    return (double)recorder->samples_written / recorder->sample_rate;
}
