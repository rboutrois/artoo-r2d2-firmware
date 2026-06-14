#pragma once
#include <Arduino.h>
#include "types.h"

void    actions_init();
bool    action_save(const CustomAction& a);
bool    action_delete(int id);
bool    action_run(int id, const ArtooConfig* cfg);
int     actions_get_all(CustomAction* out, int maxCount);

// Sequences — stored as ordered arrays of {action_id, delay_ms}
bool    sequence_save(int id, const char* name, const char* jsonSteps);
bool    sequence_delete(int id);
bool    sequence_run(int id, const ArtooConfig* cfg);
String  sequences_get_json();
String  actions_get_json();
