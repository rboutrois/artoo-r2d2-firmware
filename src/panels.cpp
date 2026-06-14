#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include "panels.h"
#include "config.h"   // PIN_I2C_SDA, PIN_I2C_SCL

// Two PCA9685 boards on the same I2C bus (dome panel servos)
static Adafruit_PWMServoDriver s_pca[2] = {
    Adafruit_PWMServoDriver(PANEL_PCA_ADDR_0),
    Adafruit_PWMServoDriver(PANEL_PCA_ADDR_1),
};

// Per-servo calibration, kept in RAM (TODO: persist via config.cpp / NVS)
static int s_closed[PANEL_COUNT];
static int s_open[PANEL_COUNT];

void panels_init() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    for (int b = 0; b < 2; b++) {
        s_pca[b].begin();
        s_pca[b].setPWMFreq(PANEL_PWM_FREQ);
    }
    // Load default calibration until real values are set
    for (int i = 0; i < PANEL_COUNT; i++) {
        s_closed[i] = PANEL_DEFAULT_CLOSED;
        s_open[i]   = PANEL_DEFAULT_OPEN;
    }
    panels_all_close();   // park everything closed at startup
}

// servo 0..15 -> board 0 (0x40), 16..31 -> board 1 (0x41)
void panel_set(int servo, bool open) {
    if (servo < 0 || servo >= PANEL_COUNT) return;
    int board = servo / 16;
    int ch    = servo % 16;
    int count = open ? s_open[servo] : s_closed[servo];
    s_pca[board].setPWM(ch, 0, count);
}

void panels_all_close() {
    for (int i = 0; i < PANEL_COUNT; i++) panel_set(i, false);
}

// Send a raw 12-bit PWM value to one servo — use this to find the
// closed/open values during calibration, then store them with
// panel_set_calibration(). Never push a servo into a mechanical hard stop.
void panel_test(int servo, int pwmCount) {
    if (servo < 0 || servo >= PANEL_COUNT) return;
    s_pca[servo / 16].setPWM(servo % 16, 0, pwmCount);
}

void panel_set_calibration(int servo, int closedCount, int openCount) {
    if (servo < 0 || servo >= PANEL_COUNT) return;
    s_closed[servo] = closedCount;
    s_open[servo]   = openCount;
}

int panel_get_closed(int servo) {
    return (servo >= 0 && servo < PANEL_COUNT) ? s_closed[servo] : 0;
}

int panel_get_open(int servo) {
    return (servo >= 0 && servo < PANEL_COUNT) ? s_open[servo] : 0;
}

// TODO (séquences) : ajouter ici des enchaînements (cascade, battement...).
// Les faire en NON bloquant avec millis() — voir dome.cpp pour le modèle —
// car la règle du projet interdit delay() dans la loop().
