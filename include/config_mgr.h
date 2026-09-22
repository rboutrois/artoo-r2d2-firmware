#pragma once
#include "types.h"

void config_init(ArtooConfig* cfg);
void config_save(const ArtooConfig* cfg);
void config_reset();   // erase all NVS keys (reverts to defaults on next boot)

// Panel calibration is stored as two blobs rather than 64 separate keys.
// Values left untouched keep whatever the caller put in the arrays.
void config_load_panels(int* closed, int* open, int count);
void config_save_panels(const int* closed, const int* open, int count);
