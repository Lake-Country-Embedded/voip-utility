/*
 * voip-utility - SIP VoIP Testing Utility
 * Audio file player
 */

#ifndef VU_PLAYER_H
#define VU_PLAYER_H

#include "util/error.h"
#include <stdbool.h>

typedef struct vu_player vu_player_t;

vu_player_t *vu_player_create(const char *path, bool loop);
void vu_player_destroy(vu_player_t *player);
vu_error_t vu_player_start(vu_player_t *player);
void vu_player_stop(vu_player_t *player);
bool vu_player_is_playing(const vu_player_t *player);

#endif /* VU_PLAYER_H */
