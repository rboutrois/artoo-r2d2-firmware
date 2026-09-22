#pragma once
#include "types.h"

// =============================================================================
// PANELS — dome panel servos (and holoprojectors)
// =============================================================================
// Driven through TWO PCA9685 boards on the I2C bus that goes up to the dome
// through the slipring:
//   - servo 0..15  -> board 0, address 0x40
//   - servo 16..31 -> board 1, address 0x41
//
// Nothing moves unless the panels are enabled in the config: the dome is not
// always wired, and a servo driven into a mechanical hard stop is a dead servo.

#define PANEL_COUNT           32      // 2 x PCA9685 (16 channels each)
#define PANEL_PCA_ADDR_0      0x40
#define PANEL_PCA_ADDR_1      0x41
#define PANEL_PWM_FREQ        50      // Hz, standard servo frame rate

// 12-bit PWM counts (0..4095), TO BE CALIBRATED servo by servo.
// Reference: at 50 Hz, ~150 is about 1 ms and ~600 is about 2 ms.
#define PANEL_DEFAULT_CLOSED  150
#define PANEL_DEFAULT_OPEN    600

#define PANEL_WAVE_STEP_MS    120     // default delay between panels in a wave

void panels_init(const ArtooConfig* cfg);
void panels_update();                          // advances running animations
void panel_set(int servo, bool open);          // one panel, open or closed
void panels_all_close();
void panels_all_open();

// Cascade: opens (or closes) the panels one after another. Non-blocking, so a
// wave never stalls the RC link or the emergency stop.
void panels_wave(bool open, int stepMs);
bool panels_busy();

// Raw 12-bit value on one servo, for calibration. Never push a servo into a
// mechanical hard stop.
void panel_test(int servo, int pwmCount);

void panel_set_calibration(int servo, int closedCount, int openCount);
void panels_save_calibration();                // persists to NVS via config.cpp
int  panel_get_closed(int servo);
int  panel_get_open(int servo);
