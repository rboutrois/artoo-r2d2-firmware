#include <Arduino.h>
#include <ESP32Servo.h>
#include "dome.h"
#include "config.h"

static Servo         s_servo;
static float         s_currentSpeed = 0.0f;   // -100..+100, smoothed
static float         s_targetSpeed  = 0.0f;
static unsigned long s_lastUpdate   = 0;
static unsigned long s_nextRandom   = 0;
static unsigned long s_moveUntil    = 0;   // end of a timed move (0 = none)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void writeServo(const ArtooConfig* cfg) {
    float range = (float)(cfg->domeSpeedRange * 20);
    float angle = (float)cfg->domeCenter + (s_currentSpeed / 100.0f) * range;
    if (angle < (float)cfg->domeMin) angle = (float)cfg->domeMin;
    if (angle > (float)cfg->domeMax) angle = (float)cfg->domeMax;
    s_servo.write((int)angle);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void dome_init(const ArtooConfig* cfg) {
    // 1000–2000 µs is the standard ESC range; neutral (90°) = 1500 µs
    s_servo.attach(PIN_DOME_PWM, 1000, 2000);
    s_servo.write(cfg->domeCenter);   // neutral — starts ESC arming sequence
    // ESC arming requires the neutral signal to be held for ~2 s.
    // This delay runs in setup() so it is acceptable.
    delay(2000);
    s_lastUpdate = millis();
    s_nextRandom = millis() + (unsigned long)cfg->domeMinTime * 1000UL;
}

void dome_set_speed(int speed) {
    if (speed >  100) speed =  100;
    if (speed < -100) speed = -100;
    s_targetSpeed = (float)speed;
    s_moveUntil   = 0;   // an explicit speed request cancels any timed move
}

// Spin for durationMs then stop by itself. Without this a "move dome" action
// with a duration would start the dome and never end it.
void dome_set_speed_for(int speed, int durationMs) {
    dome_set_speed(speed);
    if (durationMs > 0) s_moveUntil = millis() + (unsigned long)durationMs;
}

void dome_stop() {
    s_targetSpeed  = 0.0f;
    s_currentSpeed = 0.0f;
    s_moveUntil    = 0;
}

void dome_update(const ArtooConfig* cfg, ArtooStatus* status) {
    unsigned long now = millis();
    float dt = (float)(now - s_lastUpdate) / 1000.0f;
    s_lastUpdate = now;

    // Latched emergency stop: dome dead, nothing else runs
    if (status->estop) {
        s_targetSpeed  = 0.0f;
        s_currentSpeed = 0.0f;
        s_moveUntil    = 0;
        writeServo(cfg);
        status->domeSpeed = 0;
        return;
    }

    // Timed move expired
    if (s_moveUntil != 0 && (long)(now - s_moveUntil) >= 0) {
        s_moveUntil   = 0;
        s_targetSpeed = 0.0f;
    }

    // In stationary mode the left stick controls the dome directly, unless the
    // greeter is running a scene: two sources fighting over the dome would make
    // it stutter.
    if (status->mode == MODE_STATIONARY && !cfg->randomDome && !status->greeterActive) {
        s_targetSpeed = (float)status->throttleVal / 10.0f;   // -1000..+1000 → -100..+100
    }

    // Random mode: pick new target speed after timeout
    if (cfg->randomDome && (long)(now - s_nextRandom) >= 0) {
        int range = cfg->domeSpeedRange * 20;
        s_targetSpeed = (float)random(-range, range + 1);
        unsigned long interval = (unsigned long)random(
            cfg->domeMinTime  * 1000,
            cfg->domeMaxTime  * 1000 + 1);
        s_nextRandom = now + interval;
    }

    // Smooth speed toward target using accel/decel rates (units per second)
    float step;
    if (s_currentSpeed < s_targetSpeed) {
        step = (float)cfg->domeAccel * dt;
        s_currentSpeed += step;
        if (s_currentSpeed > s_targetSpeed) s_currentSpeed = s_targetSpeed;
    } else if (s_currentSpeed > s_targetSpeed) {
        step = (float)cfg->domeDecel * dt;
        s_currentSpeed -= step;
        if (s_currentSpeed < s_targetSpeed) s_currentSpeed = s_targetSpeed;
    }

    writeServo(cfg);
    status->domeSpeed = (int)s_currentSpeed;
}
