/*
 * voip-utility - SIP VoIP Testing Utility
 * WAV recorder
 */

#ifndef VU_RECORDER_H
#define VU_RECORDER_H

#include "util/error.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct vu_recorder vu_recorder_t;

vu_recorder_t *vu_recorder_create(const char *path, uint32_t sample_rate, int channels);
void vu_recorder_destroy(vu_recorder_t *recorder);
vu_error_t vu_recorder_write(vu_recorder_t *recorder, const int16_t *samples, size_t count);
double vu_recorder_get_duration(const vu_recorder_t *recorder);

#endif /* VU_RECORDER_H */
