#pragma once
#include "types.h"

void sound_init(const ArtooConfig* cfg);
void sound_update(const ArtooConfig* cfg, ArtooStatus* status);
void sound_play(int track);
void sound_play_random();
void sound_stop();
void sound_set_volume(int volume);   // 0..30
