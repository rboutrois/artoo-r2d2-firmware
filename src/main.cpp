// =============================================================================
// ARTOO R2D2 CONTROLLER — main.cpp
// =============================================================================
// Original firmware author: Steve Wagg ("Artoo Inventions")
// Reconstruction: reverse engineering of Artoo-v1.1_Beta_Full.bin
// See spec.md for the full specification.
//
// Structure:
//   setup() — initializes all modules in order
//   loop()  — calls each module's periodic tasks
//
// All delays are non-blocking (millis()). Never use delay() here.
// =============================================================================

#include <Arduino.h>
#include "config.h"
#include "types.h"

#include "config_mgr.h"
#include "wifi_server.h"
#include "sbus_receiver.h"
#include "hoverboard.h"
#include "dome.h"
#include "sound.h"
#include "arms.h"
#include "panels.h"
#include "actions.h"
#include "buttons.h"

// Global state shared across modules
ArtooConfig gConfig;
ArtooStatus gStatus;

void setup() {
    Serial.begin(115200);
    Serial.println("Artoo R2D2 Controller booting...");

    // 1. Load NVS config (Preferences)
    config_init(&gConfig);
    randomSeed(esp_random());   // true hardware RNG

    // 2. Dome ESC — attach PWM first so the ESC sees neutral (1500 µs) during
    //    the WiFi init below (~1-2 s), which acts as the arming delay.
    dome_init(&gConfig);

    // 3. WiFi Access Point + WebServer
    wifi_init(&gConfig, &gStatus);

    // 4. SBUS receiver
    sbus_init(gConfig.receiverMode);

    // 5. Hoverboard UART
    hoverboard_init();

    // 6. Sound DY-SV5W
    sound_init(&gConfig);

    // 7. Arms PCA9685
    arms_init(&gConfig);

    // 7b. Dome panel servos (two PCA9685 boards: 0x40 + 0x41)
    panels_init();

    // 8. Custom actions & sequences (SPIFFS JSON)
    actions_init();

    Serial.println("Artoo ready.");
}

void loop() {
    // Read RC input (SBUS or PWM) + detect button edges
    sbus_update(&gStatus);
    buttons_update(&gConfig, &gStatus);

    // Send hoverboard commands every 100 ms (stops automatically in stationary mode)
    hoverboard_update(&gConfig, &gStatus);

    // Dome control (accel/decel + random + stationary RC input)
    dome_update(&gConfig, &gStatus);

    // Random sounds timer
    sound_update(&gConfig, &gStatus);

    // WebServer (handle client)
    wifi_handle_client();
}
