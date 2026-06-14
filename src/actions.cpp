#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "actions.h"
#include "config.h"
#include "dome.h"
#include "sound.h"
#include "arms.h"

// ---------------------------------------------------------------------------
// SPIFFS JSON helpers
// ---------------------------------------------------------------------------

static String readFile(const char* path) {
    if (!SPIFFS.exists(path)) return "[]";
    File f = SPIFFS.open(path, "r");
    if (!f) return "[]";
    String s = f.readString();
    f.close();
    return s;
}

static bool writeFile(const char* path, const String& content) {
    File f = SPIFFS.open(path, "w");
    if (!f) return false;
    f.print(content);
    f.close();
    return true;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void actions_init() {
    // SPIFFS is already mounted by wifi_server; ensure files exist
    if (!SPIFFS.exists(SPIFFS_ACTIONS_FILE))   writeFile(SPIFFS_ACTIONS_FILE,   "[]");
    if (!SPIFFS.exists(SPIFFS_SEQUENCES_FILE)) writeFile(SPIFFS_SEQUENCES_FILE, "[]");
}

// ---------------------------------------------------------------------------
// Custom Actions
// ---------------------------------------------------------------------------

String actions_get_json() {
    return readFile(SPIFFS_ACTIONS_FILE);
}

int actions_get_all(CustomAction* out, int maxCount) {
    JsonDocument doc;
    if (deserializeJson(doc, readFile(SPIFFS_ACTIONS_FILE)) != DeserializationError::Ok) return 0;
    JsonArray arr = doc.as<JsonArray>();
    int n = 0;
    for (JsonObject obj : arr) {
        if (n >= maxCount) break;
        out[n].id     = obj["id"]     | 0;
        out[n].type   = (ActionType)(obj["type"] | 0);
        out[n].param1 = obj["param1"] | 0;
        out[n].param2 = obj["param2"] | 0;
        strlcpy(out[n].name, obj["name"] | "", sizeof(out[n].name));
        n++;
    }
    return n;
}

bool action_save(const CustomAction& a) {
    JsonDocument doc;
    deserializeJson(doc, readFile(SPIFFS_ACTIONS_FILE));
    JsonArray arr = doc.as<JsonArray>();

    // Update existing entry or append
    bool found = false;
    for (JsonObject obj : arr) {
        if ((int)(obj["id"] | -1) == a.id) {
            obj["name"]   = a.name;
            obj["type"]   = (int)a.type;
            obj["param1"] = a.param1;
            obj["param2"] = a.param2;
            found = true;
            break;
        }
    }
    if (!found) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]     = a.id;
        obj["name"]   = a.name;
        obj["type"]   = (int)a.type;
        obj["param1"] = a.param1;
        obj["param2"] = a.param2;
    }

    String out;
    serializeJson(doc, out);
    return writeFile(SPIFFS_ACTIONS_FILE, out);
}

bool action_delete(int id) {
    JsonDocument doc;
    deserializeJson(doc, readFile(SPIFFS_ACTIONS_FILE));
    JsonArray src = doc.as<JsonArray>();

    JsonDocument result;
    JsonArray dst = result.to<JsonArray>();
    for (JsonObject obj : src) {
        if ((int)(obj["id"] | -1) != id) dst.add(obj);
    }

    String out;
    serializeJson(result, out);
    return writeFile(SPIFFS_ACTIONS_FILE, out);
}

bool action_run(int id, const ArtooConfig* cfg) {
    JsonDocument doc;
    deserializeJson(doc, readFile(SPIFFS_ACTIONS_FILE));
    for (JsonObject obj : doc.as<JsonArray>()) {
        if ((int)(obj["id"] | -1) != id) continue;
        ActionType type = (ActionType)(obj["type"] | 0);
        int p1 = obj["param1"] | 0;
        int p2 = obj["param2"] | 0;
        switch (type) {
            case ACTION_MOVE_DOME:   dome_set_speed(p1);                   break;
            case ACTION_PLAY_SOUND:  sound_play(p1);                       break;
            case ACTION_OPEN_ARM1:   arms_set(1, true);                    break;
            case ACTION_CLOSE_ARM1:  arms_set(1, false);                   break;
            case ACTION_OPEN_ARM2:   arms_set(2, true);                    break;
            case ACTION_CLOSE_ARM2:  arms_set(2, false);                   break;
            case ACTION_SET_VOLUME:  sound_set_volume(p1);                 break;
        }
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Sequences
// ---------------------------------------------------------------------------

String sequences_get_json() {
    return readFile(SPIFFS_SEQUENCES_FILE);
}

bool sequence_save(int id, const char* name, const char* jsonSteps) {
    JsonDocument doc;
    deserializeJson(doc, readFile(SPIFFS_SEQUENCES_FILE));
    JsonArray arr = doc.as<JsonArray>();

    JsonDocument stepsDoc;
    deserializeJson(stepsDoc, jsonSteps);

    bool found = false;
    for (JsonObject obj : arr) {
        if ((int)(obj["id"] | -1) == id) {
            obj["name"]  = name;
            obj["steps"] = stepsDoc.as<JsonArray>();
            found = true;
            break;
        }
    }
    if (!found) {
        JsonObject obj = arr.add<JsonObject>();
        obj["id"]    = id;
        obj["name"]  = name;
        obj["steps"] = stepsDoc.as<JsonArray>();
    }

    String out;
    serializeJson(doc, out);
    return writeFile(SPIFFS_SEQUENCES_FILE, out);
}

bool sequence_delete(int id) {
    JsonDocument doc;
    deserializeJson(doc, readFile(SPIFFS_SEQUENCES_FILE));

    JsonDocument result;
    JsonArray dst = result.to<JsonArray>();
    for (JsonObject obj : doc.as<JsonArray>()) {
        if ((int)(obj["id"] | -1) != id) dst.add(obj);
    }

    String out;
    serializeJson(result, out);
    return writeFile(SPIFFS_SEQUENCES_FILE, out);
}

bool sequence_run(int id, const ArtooConfig* cfg) {
    JsonDocument doc;
    deserializeJson(doc, readFile(SPIFFS_SEQUENCES_FILE));
    for (JsonObject seq : doc.as<JsonArray>()) {
        if ((int)(seq["id"] | -1) != id) continue;
        for (JsonObject step : seq["steps"].as<JsonArray>()) {
            int actionId = step["action_id"] | -1;
            int delayMs  = step["delay_ms"]  | 0;
            if (actionId >= 0) action_run(actionId, cfg);
            if (delayMs  >  0) delay(delayMs);   // blocking — sequences are short
        }
        return true;
    }
    return false;
}
