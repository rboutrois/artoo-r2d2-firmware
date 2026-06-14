#pragma once
#include "types.h"

void hoverboard_init();
void hoverboard_update(const ArtooConfig* cfg, ArtooStatus* status);
void hoverboard_send_stop();
