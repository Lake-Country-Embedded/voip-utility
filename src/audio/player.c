/*
 * voip-utility - SIP VoIP Testing Utility
 * Audio file player (stub implementation)
 */

#include "audio/player.h"
#include "util/log.h"
#include <stdlib.h>
#include <string.h>

struct vu_player {
    char path[512];
    bool loop;
    bool playing;
};

vu_player_t *vu_player_create(const char *path, bool loop)
{
    if (!path) return NULL;

    vu_player_t *player = calloc(1, sizeof(vu_player_t));
    if (!player) return NULL;

    strncpy(player->path, path, sizeof(player->path) - 1);
    player->loop = loop;

    VU_LOG_DEBUG("Created player for: %s (loop=%d)", path, loop);
    return player;
}

void vu_player_destroy(vu_player_t *player)
{
    if (!player) return;
    vu_player_stop(player);
    free(player);
}

vu_error_t vu_player_start(vu_player_t *player)
{
    if (!player) return VU_ERR_INVALID_ARG;
    /* TODO: Implement with PJMEDIA WAV player */
    player->playing = true;
    VU_LOG_INFO("Playing: %s", player->path);
    return VU_OK;
}

void vu_player_stop(vu_player_t *player)
{
    if (!player) return;
    player->playing = false;
}

bool vu_player_is_playing(const vu_player_t *player)
{
    return player && player->playing;
}
