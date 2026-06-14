#pragma once
#include "types.h"

void dome_init(const ArtooConfig* cfg);
void dome_update(const ArtooConfig* cfg, ArtooStatus* status);
void dome_set_speed(int speed);   // manual target: -100..+100
void dome_stop();                 // immediate stop (emergency)
