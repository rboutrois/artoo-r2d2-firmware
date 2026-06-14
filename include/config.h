#pragma once

// =============================================================================
// ARTOO R2D2 CONTROLLER — CONFIG & DEFAULT VALUES
// =============================================================================
// All hardware constants and UI default values.
// Source: reverse engineering of Artoo-v1.1_Beta_Full.bin + Steve Wagg's UI screenshots.

// -----------------------------------------------------------------------------
// PINS
// -----------------------------------------------------------------------------
#define PIN_SBUS1           15      // SBUS receiver 1 (Serial2 RX)
#define PIN_SBUS2           13      // SBUS receiver 2 (Dual SBUS mode)

// PWM multi-pin mode (Standard PWM, 6 channels)
#define PIN_PWM_CH1         15
#define PIN_PWM_CH2         13
#define PIN_PWM_CH3         2
#define PIN_PWM_CH4         4
#define PIN_PWM_CH5         12
#define PIN_PWM_CH6         27

// Hoverboard UART — S1 PCB (Serial1)
// NOTE: RX/TX swapped vs initial estimate
#define PIN_HOVER_RX        17      // S1 RX
#define PIN_HOVER_TX        16      // S1 TX

// Sound DY-SV5W — S2 PCB
#define PIN_SOUND_RX        35      // S2 RX
#define PIN_SOUND_TX        26      // S2 TX

// Dome Control — S3 PCB (serial vers ESC ou Astropixels)
#define PIN_DOME_SERIAL_RX  34      // S3 RX
#define PIN_DOME_SERIAL_TX  33      // S3 TX

// Dome ESC PWM — connecteur DOME PCB
// Dome ESC PWM — connecteur DOME PCB
#define PIN_DOME_PWM        25

// Servo outputs directs (ARM1–ARM5)
#define PIN_ARM1            23
#define PIN_ARM2            5
#define PIN_ARM3            19
#define PIN_ARM4            18
#define PIN_ARM5            32

// I2C (PCA9685 — arm servos)
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22
#define PCA9685_ADDR        0x40

// RC input channels (Standard PWM mode)
// CH1=TDO(15), CH2=TCK(13), CH3=IO2, CH4=IO4, CH5=TDI(12), CH6=IO27
// Matches firmware string: "Channels 1-6 to pins 15,13,2,4,12,27" ✓

// -----------------------------------------------------------------------------
// HOVERBOARD — HOVERSERIAL PROTOCOL
// -----------------------------------------------------------------------------
#define HOVER_SERIAL_BAUD   115200
#define HOVER_START_FRAME   0xABCD
#define HOVER_SEND_INTERVAL 100     // ms between frames

// -----------------------------------------------------------------------------
// RC RECEIVER
// -----------------------------------------------------------------------------
#define SBUS_BAUD           100000
// Receiver modes
#define RECEIVER_PWM        0
#define RECEIVER_SBUS       1
#define RECEIVER_DUAL_SBUS  2

// SBUS channels (0-based index into channels[])
#define CH_THROTTLE         1   // Channel 2 — forward/reverse
#define CH_STEER            0   // Channel 1 — steering
#define CH_BTN3             2   // Channel 3 — programmable button
#define CH_BTN4             3   // Channel 4 — programmable button
#define CH_BTN5             4   // Channel 5 — programmable button
#define CH_BTN6             5   // Channel 6 — programmable button

// SBUS range
#define SBUS_MIN            172
#define SBUS_MAX            1811
#define SBUS_MID            992

// -----------------------------------------------------------------------------
// OPERATION MODES
// -----------------------------------------------------------------------------
#define MODE_DRIVING        0
#define MODE_STATIONARY     1

// -----------------------------------------------------------------------------
// DEFAULT VALUES — MOTORS
// -----------------------------------------------------------------------------
#define DEFAULT_SPEED       300     // Top speed (range 50–1000)
#define DEFAULT_STEER       155     // Top steer
#define DEFAULT_MOTOR_L_FWD false   // Left motor reversed by default
#define DEFAULT_MOTOR_R_FWD false   // Right motor reversed by default

// -----------------------------------------------------------------------------
// DEFAULT VALUES — DOME
// -----------------------------------------------------------------------------
#define DEFAULT_DOME_CENTER     90      // Stop position (servo degrees)
#define DEFAULT_DOME_MIN        50      // Full reverse
#define DEFAULT_DOME_MAX        130     // Full forward
#define DEFAULT_DOME_SPEED_RANGE 3      // Level 1–5 (UI slider)
#define DEFAULT_DOME_ACCEL      15      // deg/s²
#define DEFAULT_DOME_DECEL      3       // deg/s²
#define DEFAULT_DOME_RANDOM     false
#define DEFAULT_DOME_MIN_TIME   10      // seconds
#define DEFAULT_DOME_MAX_TIME   60      // seconds

// Servo range per speed level (level 1 = gentle, level 5 = max)
// Level N: center ± (N * 20)
// Level 1 → 70–110, Level 2 → 60–120, ..., Level 5 → 10–170

// -----------------------------------------------------------------------------
// DEFAULT VALUES — SOUND (DY-SV5W)
// -----------------------------------------------------------------------------
#define SOUND_BAUD              9600
#define DEFAULT_VOLUME          25      // 0–30
#define DEFAULT_STARTUP_SOUND   1
#define DEFAULT_RANDOM_SOUNDS   false
#define DEFAULT_SOUND_MIN_TIME  4       // seconds
#define DEFAULT_SOUND_MAX_TIME  40      // seconds

// DY-SV5W protocol (hex commands)
// Play track N  : { 0xAA, 0x07, 0x02, HIGH(N), LOW(N), CHECKSUM }
// Stop          : { 0xAA, 0x04, 0x00, 0xAE }
// Set Volume N  : { 0xAA, 0x13, 0x01, N, CHECKSUM }

// -----------------------------------------------------------------------------
// DEFAULT VALUES — ARMS (PCA9685)
// -----------------------------------------------------------------------------
#define DEFAULT_ARM1_OPEN_POS   60      // degrees
#define DEFAULT_ARM1_CLOSED_POS 110     // degrees
#define DEFAULT_ARM1_MIN_PULSE  1000    // µs
#define DEFAULT_ARM1_MAX_PULSE  2000    // µs

#define DEFAULT_ARM2_OPEN_POS   60
#define DEFAULT_ARM2_CLOSED_POS 110
#define DEFAULT_ARM2_MIN_PULSE  1000
#define DEFAULT_ARM2_MAX_PULSE  2000

// -----------------------------------------------------------------------------
// WIFI
// -----------------------------------------------------------------------------
#define DEFAULT_WIFI_SSID       "ArtooR2D2"
#define DEFAULT_WIFI_PASSWORD   "r2d2artoo"

// -----------------------------------------------------------------------------
// NVS (Preferences)
// -----------------------------------------------------------------------------
#define NVS_NAMESPACE           "artoo"

// -----------------------------------------------------------------------------
// SPIFFS — JSON files
// -----------------------------------------------------------------------------
#define SPIFFS_ACTIONS_FILE     "/custom_actions.json"
#define SPIFFS_SEQUENCES_FILE   "/sequences.json"
