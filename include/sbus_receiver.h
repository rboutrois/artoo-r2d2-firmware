#pragma once
#include "types.h"

void sbus_init(int mode);
void sbus_update(ArtooStatus* status);
bool sbus_failsafe();
bool sbus_button_just_pressed(int ch);  // true once on rising edge
bool sbus_button_state(int ch);         // current high/low level
