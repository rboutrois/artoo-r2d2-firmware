#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "wifi_server.h"
#include "config_mgr.h"
#include "config.h"
#include "hoverboard.h"
#include "dome.h"
#include "sound.h"
#include "arms.h"
#include "actions.h"
#include "sbus_receiver.h"
#include "greeter.h"
#include "panels.h"

static WebServer    server(80);
static ArtooConfig* pConfig;
static ArtooStatus* pStatus;

// ---------------------------------------------------------------------------
// Static file serving (SPIFFS)
// ---------------------------------------------------------------------------

static bool writeFile_helper(const char* path, const String& content) {
    File f = SPIFFS.open(path, "w");
    if (!f) return false;
    f.print(content);
    f.close();
    return true;
}

static void serveFile(const char* path, const char* mime) {
    if (!SPIFFS.exists(path)) { server.send(404, "text/plain", "Not found"); return; }
    File f = SPIFFS.open(path, "r");
    server.streamFile(f, mime);
    f.close();
}

static void handleRoot()   { serveFile("/index.html",  "text/html"); }
static void handleManual() { serveFile("/manual.html", "text/html"); }
static void handleConfig() { serveFile("/config.html", "text/html"); }
static void handleShow()   { serveFile("/show.html",   "text/html"); }
static void handlePanelsPage() { serveFile("/panels.html", "text/html"); }

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void sendOk() {
    server.send(200, "application/json", "{\"ok\":true}");
}

static void sendError(int code, const char* msg) {
    String s = "{\"error\":\"";
    s += msg;
    s += "\"}";
    server.send(code, "application/json", s);
}

// ---------------------------------------------------------------------------
// GET /status
// ---------------------------------------------------------------------------

static void handleStatus() {
    JsonDocument doc;
    doc["speed"]               = pConfig->speed;
    doc["steer"]               = pConfig->steer;
    doc["dome_speed"]          = pStatus->domeSpeed;
    doc["dome_center"]         = pConfig->domeCenter;
    doc["dome_min"]            = pConfig->domeMin;
    doc["dome_max"]            = pConfig->domeMax;
    doc["dome_accel"]          = pConfig->domeAccel;
    doc["dome_decel"]          = pConfig->domeDecel;
    doc["dome_min_time"]       = pConfig->domeMinTime;
    doc["dome_max_time"]       = pConfig->domeMaxTime;
    doc["random_sounds"]       = pConfig->randomSounds ? 1 : 0;
    doc["random_dome"]         = pConfig->randomDome   ? 1 : 0;
    doc["left_motor_forward"]  = pConfig->leftMotorFwd  ? 1 : 0;
    doc["right_motor_forward"] = pConfig->rightMotorFwd ? 1 : 0;
    doc["startup_sound"]       = pConfig->startupSound;
    doc["steer_val"]           = pStatus->steerVal;
    doc["throttle_val"]        = pStatus->throttleVal;
    doc["second_steer_val"]    = pStatus->secondSteerVal;
    doc["second_throttle_val"] = pStatus->secondThrottleVal;
    doc["battery"]             = pStatus->batteryVoltage;
    doc["volume"]              = pConfig->volume;
    doc["mode"]                = pStatus->mode;
    doc["receiver_mode"]       = pConfig->receiverMode;
    doc["estop"]               = pStatus->estop      ? 1 : 0;
    doc["rc_ok"]               = pStatus->rcFailsafe ? 0 : 1;
    doc["crowd_limit"]         = pConfig->crowdLimit ? 1 : 0;
    doc["crowd_speed"]         = pConfig->crowdSpeed;
    doc["sequence_running"]    = sequence_is_running() ? 1 : 0;
    doc["greeter_on"]          = greeter_enabled()      ? 1 : 0;
    doc["greeter_active"]      = pStatus->greeterActive ? 1 : 0;
    doc["greeter_intensity"]   = pConfig->greeterIntensity;
    doc["scene_running"]       = greeter_scene_running() ? 1 : 0;
    doc["track_offset"]        = pConfig->trackOffset;
    doc["track_count"]         = pConfig->trackCount;

    JsonArray btn = doc["btn"].to<JsonArray>();
    for (int i = 0; i < 4; i++) btn.add(sbus_button_state(i) ? 1 : 0);

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

// ---------------------------------------------------------------------------
// GET /getConfig
// ---------------------------------------------------------------------------

static void handleGetConfig() {
    JsonDocument doc;
    doc["speed"]               = pConfig->speed;
    doc["steer"]               = pConfig->steer;
    doc["left_motor_forward"]  = pConfig->leftMotorFwd  ? 1 : 0;
    doc["right_motor_forward"] = pConfig->rightMotorFwd ? 1 : 0;
    doc["dome_center"]         = pConfig->domeCenter;
    doc["dome_min"]            = pConfig->domeMin;
    doc["dome_max"]            = pConfig->domeMax;
    doc["dome_speed_range"]    = pConfig->domeSpeedRange;
    doc["dome_accel"]          = pConfig->domeAccel;
    doc["dome_decel"]          = pConfig->domeDecel;
    doc["random_dome"]         = pConfig->randomDome ? 1 : 0;
    doc["dome_min_time"]       = pConfig->domeMinTime;
    doc["dome_max_time"]       = pConfig->domeMaxTime;
    doc["volume"]              = pConfig->volume;
    doc["startup_sound"]       = pConfig->startupSound;
    doc["random_sounds"]       = pConfig->randomSounds ? 1 : 0;
    doc["sound_min_time"]      = pConfig->soundMinTime;
    doc["sound_max_time"]      = pConfig->soundMaxTime;
    doc["arm1_open"]           = pConfig->arm1OpenPos;
    doc["arm1_closed"]         = pConfig->arm1ClosedPos;
    doc["arm1_min_pulse"]      = pConfig->arm1MinPulse;
    doc["arm1_max_pulse"]      = pConfig->arm1MaxPulse;
    doc["arm2_open"]           = pConfig->arm2OpenPos;
    doc["arm2_closed"]         = pConfig->arm2ClosedPos;
    doc["arm2_min_pulse"]      = pConfig->arm2MinPulse;
    doc["arm2_max_pulse"]      = pConfig->arm2MaxPulse;
    doc["receiver_mode"]       = pConfig->receiverMode;
    doc["crowd_limit"]         = pConfig->crowdLimit ? 1 : 0;
    doc["crowd_speed"]         = pConfig->crowdSpeed;
    doc["greeter_on"]          = pConfig->greeterEnabled ? 1 : 0;
    doc["greeter_intensity"]   = pConfig->greeterIntensity;
    doc["track_count"]         = pConfig->trackCount;
    doc["track_offset"]        = pConfig->trackOffset;

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

// ---------------------------------------------------------------------------
// POST /saveConfig  (JSON body — any subset of config fields)
// ---------------------------------------------------------------------------

static void handleSaveConfig() {
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
        sendError(400, "invalid JSON");
        return;
    }

    pConfig->speed          = doc["speed"]               | pConfig->speed;
    pConfig->steer          = doc["steer"]               | pConfig->steer;
    pConfig->leftMotorFwd   = (doc["left_motor_forward"]  | (pConfig->leftMotorFwd  ? 1 : 0)) != 0;
    pConfig->rightMotorFwd  = (doc["right_motor_forward"] | (pConfig->rightMotorFwd ? 1 : 0)) != 0;
    pConfig->domeCenter     = doc["dome_center"]         | pConfig->domeCenter;
    pConfig->domeMin        = doc["dome_min"]            | pConfig->domeMin;
    pConfig->domeMax        = doc["dome_max"]            | pConfig->domeMax;
    pConfig->domeSpeedRange = doc["dome_speed_range"]    | pConfig->domeSpeedRange;
    pConfig->domeAccel      = doc["dome_accel"]          | pConfig->domeAccel;
    pConfig->domeDecel      = doc["dome_decel"]          | pConfig->domeDecel;
    pConfig->randomDome     = (doc["random_dome"]        | (pConfig->randomDome  ? 1 : 0)) != 0;
    pConfig->domeMinTime    = doc["dome_min_time"]       | pConfig->domeMinTime;
    pConfig->domeMaxTime    = doc["dome_max_time"]       | pConfig->domeMaxTime;
    pConfig->volume         = doc["volume"]              | pConfig->volume;
    pConfig->startupSound   = doc["startup_sound"]       | pConfig->startupSound;
    pConfig->randomSounds   = (doc["random_sounds"]      | (pConfig->randomSounds ? 1 : 0)) != 0;
    pConfig->soundMinTime   = doc["sound_min_time"]      | pConfig->soundMinTime;
    pConfig->soundMaxTime   = doc["sound_max_time"]      | pConfig->soundMaxTime;
    pConfig->arm1OpenPos    = doc["arm1_open"]           | pConfig->arm1OpenPos;
    pConfig->arm1ClosedPos  = doc["arm1_closed"]         | pConfig->arm1ClosedPos;
    pConfig->arm1MinPulse   = doc["arm1_min_pulse"]      | pConfig->arm1MinPulse;
    pConfig->arm1MaxPulse   = doc["arm1_max_pulse"]      | pConfig->arm1MaxPulse;
    pConfig->arm2OpenPos    = doc["arm2_open"]           | pConfig->arm2OpenPos;
    pConfig->arm2ClosedPos  = doc["arm2_closed"]         | pConfig->arm2ClosedPos;
    pConfig->arm2MinPulse   = doc["arm2_min_pulse"]      | pConfig->arm2MinPulse;
    pConfig->arm2MaxPulse   = doc["arm2_max_pulse"]      | pConfig->arm2MaxPulse;
    pConfig->receiverMode   = doc["receiver_mode"]       | pConfig->receiverMode;
    pConfig->crowdLimit     = (doc["crowd_limit"]        | (pConfig->crowdLimit ? 1 : 0)) != 0;
    pConfig->crowdSpeed     = doc["crowd_speed"]         | pConfig->crowdSpeed;
    pConfig->greeterEnabled = (doc["greeter_on"]         | (pConfig->greeterEnabled ? 1 : 0)) != 0;
    pConfig->greeterIntensity = doc["greeter_intensity"] | pConfig->greeterIntensity;
    pConfig->trackCount     = doc["track_count"]         | pConfig->trackCount;
    pConfig->trackOffset    = doc["track_offset"]        | pConfig->trackOffset;

    sound_set_track_count(pConfig->trackCount);
    greeter_set_enabled(pConfig->greeterEnabled);
    config_save(pConfig);
    sendOk();
}

// ---------------------------------------------------------------------------
// GET /getWiFiConfig
// POST /setWiFiConfig  ssid, password
// ---------------------------------------------------------------------------

static void handleGetWiFiConfig() {
    JsonDocument doc;
    doc["ssid"]     = pConfig->wifiSSID;
    doc["password"] = pConfig->wifiPassword;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handleSetWiFiConfig() {
    String ssid = server.arg("ssid");
    String pass = server.arg("password");
    if (ssid.isEmpty()) { sendError(400, "ssid required"); return; }

    strlcpy(pConfig->wifiSSID,     ssid.c_str(), sizeof(pConfig->wifiSSID));
    strlcpy(pConfig->wifiPassword, pass.c_str(), sizeof(pConfig->wifiPassword));
    config_save(pConfig);

    WiFi.softAP(pConfig->wifiSSID, pConfig->wifiPassword);
    sendOk();
}

// ---------------------------------------------------------------------------
// Motor config endpoints
// ---------------------------------------------------------------------------

static void handleSetSpeed() {
    int v = server.arg("value").toInt();
    if (v < 50 || v > 1000) { sendError(400, "value out of range [50,1000]"); return; }
    pConfig->speed = v;
    config_save(pConfig);
    sendOk();
}

static void handleSetSteer() {
    int v = server.arg("value").toInt();
    pConfig->steer = v;
    config_save(pConfig);
    sendOk();
}

static void handleSetMode() {
    int v = server.arg("value").toInt();
    if (v != 0 && v != 1) { sendError(400, "value must be 0 or 1"); return; }
    pStatus->mode = v;
    sendOk();
}

static void handleSetReceiverMode() {
    int v = server.arg("value").toInt();
    if (v < 0 || v > 2) { sendError(400, "value must be 0, 1, or 2"); return; }
    pConfig->receiverMode = v;
    config_save(pConfig);
    sendOk();
}

static void handleSetLeftMotorDir() {
    int v = server.arg("value").toInt();
    pConfig->leftMotorFwd = (v == 0);
    config_save(pConfig);
    sendOk();
}

static void handleSetRightMotorDir() {
    int v = server.arg("value").toInt();
    pConfig->rightMotorFwd = (v == 0);
    config_save(pConfig);
    sendOk();
}

// ---------------------------------------------------------------------------
// Dome config endpoints
// ---------------------------------------------------------------------------

static void handleSetDomeCalibration() {
    pConfig->domeCenter = server.arg("center").toInt();
    pConfig->domeMin    = server.arg("min").toInt();
    pConfig->domeMax    = server.arg("max").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleSetDomeSmoothing() {
    pConfig->domeAccel = server.arg("accel").toInt();
    pConfig->domeDecel = server.arg("decel").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleSetDomeTimerSettings() {
    pConfig->domeMinTime = server.arg("min_time").toInt();
    pConfig->domeMaxTime = server.arg("max_time").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleSetRandomDome() {
    pConfig->randomDome = (server.arg("value").toInt() != 0);
    config_save(pConfig);
    sendOk();
}

static void handleSetDomeManual() {
    dome_set_speed(server.arg("speed").toInt());
    sendOk();
}

// ---------------------------------------------------------------------------
// Sound config endpoints
// ---------------------------------------------------------------------------

static void handleSetVolume() {
    int v = server.arg("value").toInt();
    if (v < 0 || v > 30) { sendError(400, "value out of range [0,30]"); return; }
    pConfig->volume = v;
    config_save(pConfig);
    sound_set_volume(v);
    sendOk();
}

static void handleSetStartupSound() {
    pConfig->startupSound = server.arg("value").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleSetRandomSounds() {
    pConfig->randomSounds = (server.arg("value").toInt() != 0);
    config_save(pConfig);
    sendOk();
}

static void handleSetTimerSettings() {
    pConfig->soundMinTime = server.arg("min_time").toInt();
    pConfig->soundMaxTime = server.arg("max_time").toInt();
    config_save(pConfig);
    sendOk();
}

static void handlePlaySound() {
    sound_play_random();
    sendOk();
}
static void handlePlaySpecificSound() {
    sound_play(server.arg("number").toInt());
    sendOk();
}

// ---------------------------------------------------------------------------
// Arm config endpoints
// ---------------------------------------------------------------------------

static void handleSetArm1Positions() {
    pConfig->arm1OpenPos   = server.arg("open_pos").toInt();
    pConfig->arm1ClosedPos = server.arg("closed_pos").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleSetArm1Pulse() {
    pConfig->arm1MinPulse = server.arg("min_pulse").toInt();
    pConfig->arm1MaxPulse = server.arg("max_pulse").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleSetArm2Positions() {
    pConfig->arm2OpenPos   = server.arg("open_pos").toInt();
    pConfig->arm2ClosedPos = server.arg("closed_pos").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleSetArm2Pulse() {
    pConfig->arm2MinPulse = server.arg("min_pulse").toInt();
    pConfig->arm2MaxPulse = server.arg("max_pulse").toInt();
    config_save(pConfig);
    sendOk();
}

static void handleTestArm1() {
    bool open = (server.arg("position") == "open");
    arms_set(1, open);
    sendOk();
}
static void handleTestArm2() {
    bool open = (server.arg("position") == "open");
    arms_set(2, open);
    sendOk();
}

// ---------------------------------------------------------------------------
// Custom actions & sequences
// ---------------------------------------------------------------------------

static void handleGetCustomActions() {
    server.send(200, "application/json", actions_get_json());
}

static void handleSaveCustomAction() {
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
        sendError(400, "invalid JSON"); return;
    }
    CustomAction a;
    a.id     = doc["id"]     | 0;
    a.type   = (ActionType)(doc["type"] | 0);
    a.param1 = doc["param1"] | 0;
    a.param2 = doc["param2"] | 0;
    strlcpy(a.name, doc["name"] | "", sizeof(a.name));
    action_save(a) ? sendOk() : sendError(500, "write failed");
}

static void handleDeleteCustomAction() {
    action_delete(server.arg("id").toInt()) ? sendOk() : sendError(404, "not found");
}

static void handleTestCustomAction() {
    action_run(server.arg("id").toInt(), pConfig) ? sendOk() : sendError(404, "not found");
}

static void handleExportCustomActions() {
    server.send(200, "application/json", actions_get_json());
}

static void handleImportCustomActions() {
    if (!writeFile_helper(SPIFFS_ACTIONS_FILE, server.arg("plain"))) {
        sendError(500, "write failed"); return;
    }
    sendOk();
}

static void handleGetSequences() {
    server.send(200, "application/json", sequences_get_json());
}

static void handleSaveSequence() {
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
        sendError(400, "invalid JSON"); return;
    }
    String steps;
    serializeJson(doc["steps"], steps);
    sequence_save(doc["id"] | 0, doc["name"] | "", steps.c_str())
        ? sendOk() : sendError(500, "write failed");
}

static void handleDeleteSequence() {
    sequence_delete(server.arg("id").toInt()) ? sendOk() : sendError(404, "not found");
}

static void handleTestSequence() {
    sequence_run(server.arg("id").toInt(), pConfig) ? sendOk() : sendError(404, "not found");
}

static void handleExportSequences() {
    server.send(200, "application/json", sequences_get_json());
}

static void handleImportSequences() {
    if (!writeFile_helper(SPIFFS_SEQUENCES_FILE, server.arg("plain"))) {
        sendError(500, "write failed"); return;
    }
    sendOk();
}

static void handleNotImplemented() {
    server.send(501, "application/json", "{\"error\":\"not implemented\"}");
}

// ---------------------------------------------------------------------------
// Button channel config
// ---------------------------------------------------------------------------

static void handleGetButtonConfig() {
    JsonDocument doc;
    JsonArray drv = doc["driving"].to<JsonArray>();
    JsonArray sta = doc["stationary"].to<JsonArray>();
    for (int i = 0; i < 4; i++) {
        JsonObject od = drv.add<JsonObject>();
        od["action"] = pConfig->btnDriving[i].action;
        od["param1"] = pConfig->btnDriving[i].param1;
        JsonObject os = sta.add<JsonObject>();
        os["action"] = pConfig->btnStationary[i].action;
        os["param1"] = pConfig->btnStationary[i].param1;
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handleSaveButtonConfig() {
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
        sendError(400, "invalid JSON"); return;
    }
    auto loadArray = [&](const char* key, BtnConfig* dst) {
        if (!doc[key].is<JsonArray>()) return;
        int i = 0;
        for (JsonObject o : doc[key].as<JsonArray>()) {
            if (i >= 4) break;
            dst[i].action = o["action"] | 0;
            dst[i].param1 = o["param1"] | 0;
            i++;
        }
    };
    loadArray("driving",    pConfig->btnDriving);
    loadArray("stationary", pConfig->btnStationary);
    config_save(pConfig);
    sendOk();
}

// ---------------------------------------------------------------------------
// Factory reset — clears NVS and reboots (all settings revert to defaults)
// ---------------------------------------------------------------------------

static void handleResetConfig() {
    config_reset();
    server.send(200, "application/json", "{\"ok\":true}");
    delay(300);
    ESP.restart();
}

// ---------------------------------------------------------------------------
// Emergency stop
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Dome panels
// ---------------------------------------------------------------------------

static void handleGetPanels() {
    JsonDocument doc;
    doc["enabled"] = pConfig->panelsEnabled ? 1 : 0;
    doc["count"]   = PANEL_COUNT;
    doc["busy"]    = panels_busy() ? 1 : 0;
    JsonArray c = doc["closed"].to<JsonArray>();
    JsonArray o = doc["open"].to<JsonArray>();
    for (int i = 0; i < PANEL_COUNT; i++) {
        c.add(panel_get_closed(i));
        o.add(panel_get_open(i));
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

// Enabling requires a reboot: the I2C bus and both PCA9685 boards are set up
// in setup(), and re-running that from a request handler is asking for trouble.
static void handleSetPanels() {
    pConfig->panelsEnabled = (server.arg("value").toInt() != 0);
    config_save(pConfig);
    server.send(200, "application/json", "{\"ok\":true,\"reboot_required\":true}");
}

static void handleTestPanel() {
    panel_test(server.arg("servo").toInt(), server.arg("pwm").toInt());
    sendOk();
}

static void handleSetPanelCalibration() {
    int servo  = server.arg("servo").toInt();
    int closed = server.arg("closed").toInt();
    int open   = server.arg("open").toInt();
    if (servo < 0 || servo >= PANEL_COUNT) { sendError(400, "bad servo"); return; }
    panel_set_calibration(servo, closed, open);
    panels_save_calibration();
    sendOk();
}

static void handlePanelSet() {
    panel_set(server.arg("servo").toInt(), server.arg("open").toInt() != 0);
    sendOk();
}

static void handlePanelsAll() {
    server.arg("open").toInt() != 0 ? panels_all_open() : panels_all_close();
    sendOk();
}

static void handlePanelsWave() {
    int step = server.hasArg("step") ? server.arg("step").toInt() : PANEL_WAVE_STEP_MS;
    panels_wave(server.arg("open").toInt() != 0, step);
    sendOk();
}

// ---------------------------------------------------------------------------
// Greeter (reception mode)
// ---------------------------------------------------------------------------

static void handleGetScenes() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < greeter_scene_count(); i++) {
        JsonObject o = arr.add<JsonObject>();
        o["id"]      = i;
        o["name"]    = greeter_scene_name(i);
        o["ambient"] = greeter_scene_is_ambient(i) ? 1 : 0;
    }
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handlePlayScene() {
    if (pStatus->estop) { sendError(409, "emergency stop latched"); return; }
    greeter_play_scene(server.arg("id").toInt()) ? sendOk() : sendError(404, "no such scene");
}

static void handleStopScene() {
    greeter_stop();
    sendOk();
}

static void handleSetGreeter() {
    pConfig->greeterEnabled = (server.arg("value").toInt() != 0);
    greeter_set_enabled(pConfig->greeterEnabled);
    config_save(pConfig);
    sendOk();
}

static void handleSetGreeterIntensity() {
    int v = server.arg("value").toInt();
    if (v < 0) v = 0;
    if (v > 2) v = 2;
    pConfig->greeterIntensity = v;
    config_save(pConfig);
    sendOk();
}

// Shifts every track number sent to the sound module. Lets us realign the
// whole bank from the phone if the card is numbered differently than expected,
// with no reflash.
static void handleSetTrackOffset() {
    pConfig->trackOffset = server.arg("value").toInt();
    config_save(pConfig);
    sendOk();
}

// Bench tool: play a file by path to work out the string format the module
// expects, without reflashing between attempts.
static void handlePlayPath() {
    String path  = server.arg("path");
    int    drive = server.hasArg("drive") ? server.arg("drive").toInt() : 1;   // 1 = SD
    if (path.isEmpty()) { sendError(400, "path required"); return; }
    sound_play_path(drive, path.c_str());
    sendOk();
}

// The stop has to LATCH. Zeroing the stick values only bought 20 ms: the next
// sbus_update() read the sticks again and the robot carried on. Once latched,
// every module refuses to drive until /clearEmergencyStop is called.
static void handleEmergencyStop() {
    pStatus->estop             = true;
    pStatus->throttleVal       = 0;
    pStatus->steerVal          = 0;
    pStatus->secondThrottleVal = 0;
    pStatus->secondSteerVal    = 0;
    sequence_stop();
    greeter_stop();
    panels_all_close();
    hoverboard_send_stop();
    dome_stop();
    sound_stop();
    sendOk();
}

static void handleClearEmergencyStop() {
    pStatus->estop = false;
    sendOk();
}

// Crowd limit: one tap to cap the top speed for an indoor event, one tap to
// get the full range back, without editing the configured top speed.
static void handleSetCrowdLimit() {
    pConfig->crowdLimit = (server.arg("value").toInt() != 0);
    config_save(pConfig);
    sendOk();
}

// ---------------------------------------------------------------------------
// OTA update
// ---------------------------------------------------------------------------

static void handleUpdateDone() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", Update.hasError() ? "UPDATE FAILED" : "OK — rebooting");
    delay(100);
    ESP.restart();
}

static void handleUpdateUpload() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("OTA: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Serial.println("OTA begin failed");
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Serial.println("OTA write error");
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!Update.end(true)) {
            Serial.println("OTA end failed");
        }
    }
}

// ---------------------------------------------------------------------------
// Route registration
// ---------------------------------------------------------------------------

static void registerRoutes() {
    // UI pages (SPIFFS)
    server.on("/",        HTTP_GET, handleRoot);
    server.on("/manual",  HTTP_GET, handleManual);
    server.on("/config",  HTTP_GET, handleConfig);
    server.on("/show",    HTTP_GET, handleShow);
    server.on("/panels",  HTTP_GET, handlePanelsPage);
    server.serveStatic("/style.css", SPIFFS, "/style.css");

    // Status & config
    server.on("/status",              HTTP_GET,  handleStatus);
    server.on("/getConfig",           HTTP_GET,  handleGetConfig);
    server.on("/saveConfig",          HTTP_POST, handleSaveConfig);
    server.on("/getWiFiConfig",       HTTP_GET,  handleGetWiFiConfig);
    server.on("/setWiFiConfig",       HTTP_POST, handleSetWiFiConfig);

    // Motor
    server.on("/setSpeed",            HTTP_POST, handleSetSpeed);
    server.on("/setSteer",            HTTP_POST, handleSetSteer);
    server.on("/setMode",             HTTP_POST, handleSetMode);
    server.on("/setReceiverMode",     HTTP_POST, handleSetReceiverMode);
    server.on("/setLeftMotorDir",     HTTP_POST, handleSetLeftMotorDir);
    server.on("/setRightMotorDir",    HTTP_POST, handleSetRightMotorDir);

    // Dome
    server.on("/setDomeManual",       HTTP_POST, handleSetDomeManual);
    server.on("/setDomeCalibration",  HTTP_POST, handleSetDomeCalibration);
    server.on("/setDomeSmoothing",    HTTP_POST, handleSetDomeSmoothing);
    server.on("/setDomeTimerSettings",HTTP_POST, handleSetDomeTimerSettings);
    server.on("/setRandomDome",       HTTP_POST, handleSetRandomDome);

    // Sound
    server.on("/setVolume",           HTTP_POST, handleSetVolume);
    server.on("/setStartupSound",     HTTP_POST, handleSetStartupSound);
    server.on("/setRandomSounds",     HTTP_POST, handleSetRandomSounds);
    server.on("/setTimerSettings",    HTTP_POST, handleSetTimerSettings);
    server.on("/playSound",           HTTP_POST, handlePlaySound);
    server.on("/playSpecificSound",   HTTP_POST, handlePlaySpecificSound);

    // Arms
    server.on("/setArm1Positions",    HTTP_POST, handleSetArm1Positions);
    server.on("/setArm1Pulse",        HTTP_POST, handleSetArm1Pulse);
    server.on("/setArm2Positions",    HTTP_POST, handleSetArm2Positions);
    server.on("/setArm2Pulse",        HTTP_POST, handleSetArm2Pulse);
    server.on("/testArm1",            HTTP_POST, handleTestArm1);
    server.on("/testArm2",            HTTP_POST, handleTestArm2);

    // Custom actions & sequences
    server.on("/getCustomActions",    HTTP_GET,  handleGetCustomActions);
    server.on("/saveCustomAction",    HTTP_POST, handleSaveCustomAction);
    server.on("/deleteCustomAction",  HTTP_POST, handleDeleteCustomAction);
    server.on("/testCustomAction",    HTTP_POST, handleTestCustomAction);
    server.on("/exportCustomActions", HTTP_POST, handleExportCustomActions);
    server.on("/importCustomActions", HTTP_POST, handleImportCustomActions);
    server.on("/getSequences",        HTTP_GET,  handleGetSequences);
    server.on("/saveSequence",        HTTP_POST, handleSaveSequence);
    server.on("/deleteSequence",      HTTP_POST, handleDeleteSequence);
    server.on("/testSequence",        HTTP_POST, handleTestSequence);
    server.on("/exportSequences",     HTTP_POST, handleExportSequences);
    server.on("/importSequences",     HTTP_POST, handleImportSequences);

    // Button channel config
    server.on("/getButtonConfig",  HTTP_GET,  handleGetButtonConfig);
    server.on("/saveButtonConfig", HTTP_POST, handleSaveButtonConfig);

    // Safety, OTA & factory reset
    server.on("/emergencyStop",       HTTP_POST, handleEmergencyStop);
    server.on("/clearEmergencyStop",  HTTP_POST, handleClearEmergencyStop);
    server.on("/getScenes",           HTTP_GET,  handleGetScenes);
    server.on("/playScene",           HTTP_POST, handlePlayScene);
    server.on("/stopScene",           HTTP_POST, handleStopScene);
    server.on("/setGreeter",          HTTP_POST, handleSetGreeter);
    server.on("/setGreeterIntensity", HTTP_POST, handleSetGreeterIntensity);
    server.on("/setTrackOffset",      HTTP_POST, handleSetTrackOffset);
    server.on("/playPath",            HTTP_POST, handlePlayPath);
    server.on("/getPanels",           HTTP_GET,  handleGetPanels);
    server.on("/setPanels",           HTTP_POST, handleSetPanels);
    server.on("/testPanel",           HTTP_POST, handleTestPanel);
    server.on("/setPanelCalibration", HTTP_POST, handleSetPanelCalibration);
    server.on("/panelSet",            HTTP_POST, handlePanelSet);
    server.on("/panelsAll",           HTTP_POST, handlePanelsAll);
    server.on("/panelsWave",          HTTP_POST, handlePanelsWave);
    server.on("/setCrowdLimit",       HTTP_POST, handleSetCrowdLimit);
    server.on("/resetConfig",         HTTP_POST, handleResetConfig);
    server.on("/update",              HTTP_POST, handleUpdateDone, handleUpdateUpload);

    server.onNotFound([]() {
        server.send(404, "application/json", "{\"error\":\"not found\"}");
    });
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void wifi_init(ArtooConfig* cfg, ArtooStatus* status) {
    pConfig = cfg;
    pStatus = status;

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed — UI will not be served");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(cfg->wifiSSID, cfg->wifiPassword);

    Serial.printf("WiFi AP: %s  IP: %s\n",
        cfg->wifiSSID,
        WiFi.softAPIP().toString().c_str());

    registerRoutes();
    server.begin();
}

void wifi_handle_client() {
    server.handleClient();
}
