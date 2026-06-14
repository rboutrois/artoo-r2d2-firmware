#pragma once
#include "types.h"

void config_init(ArtooConfig* cfg);
void config_save(const ArtooConfig* cfg);
void config_reset();   // erase all NVS keys (reverts to defaults on next boot)
