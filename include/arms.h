#pragma once
#include "types.h"

void arms_init(const ArtooConfig* cfg);
void arms_set(int arm, bool open);   // arm: 1 or 2
