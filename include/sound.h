#pragma once
#include "types.h"

void sound_init(const ArtooConfig* cfg);
void sound_update(const ArtooConfig* cfg, ArtooStatus* status);
void sound_play(int track);
void sound_play_random();
void sound_stop();
void sound_set_volume(int volume);   // 0..30
void sound_set_track_count(int n);   // how many tracks the card actually holds

// Play a file by path instead of by track number (DY-SV5W "Specified Path").
// Needed once the card holds several folders, where a plain track number is
// ambiguous. drive: 0 = USB, 1 = SD, 2 = flash.
void sound_play_path(int drive, const char* path);
