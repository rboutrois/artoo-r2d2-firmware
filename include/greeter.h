#pragma once
#include "types.h"

// =============================================================================
// GREETER — autonomous "alive" behaviour for reception duty
// =============================================================================
// The robot plays short scenes (dome + sound) on its own so it stays alive in
// front of visitors without anyone driving it. It NEVER touches the wheels:
// driving stays 100% manual, this layer only animates.
//
// It steps aside the moment the operator does anything: any stick movement
// suspends the loop, so a scene never fights a human command.

void greeter_init(const ArtooConfig* cfg);
void greeter_update(const ArtooConfig* cfg, ArtooStatus* status);

void greeter_set_enabled(bool on);
bool greeter_enabled();

// Manual trigger from the web UI or an RC button. Works even when the
// autonomous loop is off, and cancels whatever scene is playing.
bool greeter_play_scene(int id);
void greeter_stop();

int         greeter_scene_count();
const char* greeter_scene_name(int id);
bool        greeter_scene_is_ambient(int id);   // used by the idle loop
bool        greeter_scene_running();
