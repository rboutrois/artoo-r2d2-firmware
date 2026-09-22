#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "panels.h"
#include "config.h"
#include "config_mgr.h"

// Two PCA9685 boards on the same I2C bus (dome panel servos)
static Adafruit_PWMServoDriver s_pca[2] = {
    Adafruit_PWMServoDriver(PANEL_PCA_ADDR_0),
    Adafruit_PWMServoDriver(PANEL_PCA_ADDR_1),
};

static int  s_closed[PANEL_COUNT];
static int  s_open[PANEL_COUNT];
static bool s_enabled = false;

// Running wave
static int           s_waveIdx    = -1;    // -1 = idle
static bool          s_waveOpen   = false;
static int           s_waveStepMs = PANEL_WAVE_STEP_MS;
static unsigned long s_waveNextMs = 0;

void panels_init(const ArtooConfig* cfg) {
    s_enabled = cfg->panelsEnabled;

    for (int i = 0; i < PANEL_COUNT; i++) {
        s_closed[i] = PANEL_DEFAULT_CLOSED;
        s_open[i]   = PANEL_DEFAULT_OPEN;
    }
    config_load_panels(s_closed, s_open, PANEL_COUNT);

    if (!s_enabled) return;   // dome not wired: leave the bus alone

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    for (int b = 0; b < 2; b++) {
        s_pca[b].begin();
        s_pca[b].setPWMFreq(PANEL_PWM_FREQ);
    }
    panels_all_close();   // park everything closed at startup
}

// servo 0..15 -> board 0 (0x40), 16..31 -> board 1 (0x41)
void panel_set(int servo, bool open) {
    if (!s_enabled || servo < 0 || servo >= PANEL_COUNT) return;
    int count = open ? s_open[servo] : s_closed[servo];
    s_pca[servo / 16].setPWM(servo % 16, 0, count);
}

void panels_all_close() {
    s_waveIdx = -1;
    for (int i = 0; i < PANEL_COUNT; i++) panel_set(i, false);
}

void panels_all_open() {
    s_waveIdx = -1;
    for (int i = 0; i < PANEL_COUNT; i++) panel_set(i, true);
}

void panels_wave(bool open, int stepMs) {
    if (!s_enabled) return;
    s_waveOpen   = open;
    s_waveStepMs = (stepMs > 0) ? stepMs : PANEL_WAVE_STEP_MS;
    s_waveIdx    = 0;
    s_waveNextMs = millis();
}

bool panels_busy() {
    return s_waveIdx >= 0;
}

void panels_update() {
    if (s_waveIdx < 0) return;

    unsigned long now = millis();
    if ((long)(now - s_waveNextMs) < 0) return;

    panel_set(s_waveIdx, s_waveOpen);
    s_waveIdx++;
    s_waveNextMs = now + (unsigned long)s_waveStepMs;

    if (s_waveIdx >= PANEL_COUNT) s_waveIdx = -1;
}

void panel_test(int servo, int pwmCount) {
    if (!s_enabled || servo < 0 || servo >= PANEL_COUNT) return;
    if (pwmCount < 0)    pwmCount = 0;
    if (pwmCount > 4095) pwmCount = 4095;
    s_pca[servo / 16].setPWM(servo % 16, 0, pwmCount);
}

void panel_set_calibration(int servo, int closedCount, int openCount) {
    if (servo < 0 || servo >= PANEL_COUNT) return;
    s_closed[servo] = closedCount;
    s_open[servo]   = openCount;
}

void panels_save_calibration() {
    config_save_panels(s_closed, s_open, PANEL_COUNT);
}

int panel_get_closed(int servo) {
    return (servo >= 0 && servo < PANEL_COUNT) ? s_closed[servo] : 0;
}

int panel_get_open(int servo) {
    return (servo >= 0 && servo < PANEL_COUNT) ? s_open[servo] : 0;
}
