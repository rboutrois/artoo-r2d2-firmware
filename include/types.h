#pragma once
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// SHARED TYPES — Artoo R2D2 Controller
// =============================================================================

// --- Hoverboard ---

typedef struct {
    uint16_t start;
    int16_t  steer;
    int16_t  speed;
    uint16_t checksum;
} HoverCommand;

typedef struct {
    uint16_t start;
    int16_t  cmd1;
    int16_t  cmd2;
    int16_t  speedR_meas;
    int16_t  speedL_meas;
    int16_t  batVoltage;    // ×100 — divide by 100 for volts
    int16_t  boardTemp;
    uint16_t cmdLed;
    uint16_t checksum;
} HoverFeedback;

// --- Button channel actions (CH3–CH6) ---
// Must be declared before ArtooConfig which embeds BtnConfig.

typedef enum {
    BTN_NONE          = 0,
    BTN_TOGGLE_MODE   = 1,
    BTN_PLAY_RANDOM   = 2,
    BTN_PLAY_SOUND    = 3,   // param1 = track number
    BTN_OPEN_ARM1     = 4,
    BTN_CLOSE_ARM1    = 5,
    BTN_OPEN_ARM2     = 6,
    BTN_CLOSE_ARM2    = 7,
    BTN_MOVE_DOME     = 8,   // param1 = speed (-100..100)
    BTN_CUSTOM_ACTION = 9,   // param1 = custom action id
    BTN_SEQUENCE      = 10,  // param1 = sequence id
} BtnAction;

typedef struct {
    uint8_t action;   // BtnAction
    int16_t param1;
} BtnConfig;

// --- Persistent configuration ---

typedef struct {
    // Motors
    int     speed;
    int     steer;
    bool    leftMotorFwd;
    bool    rightMotorFwd;

    // Dome
    int     domeCenter;
    int     domeMin;
    int     domeMax;
    int     domeSpeedRange;
    int     domeAccel;
    int     domeDecel;
    bool    randomDome;
    int     domeMinTime;
    int     domeMaxTime;

    // Sound
    int     volume;
    int     startupSound;
    bool    randomSounds;
    int     soundMinTime;
    int     soundMaxTime;

    // Arm 1
    int     arm1OpenPos;
    int     arm1ClosedPos;
    int     arm1MinPulse;
    int     arm1MaxPulse;

    // Arm 2
    int     arm2OpenPos;
    int     arm2ClosedPos;
    int     arm2MinPulse;
    int     arm2MaxPulse;

    // RC
    int     receiverMode;   // 0=PWM, 1=SBUS, 2=DualSBUS

    // WiFi
    char    wifiSSID[32];
    char    wifiPassword[64];

    // Button channel assignments (index 0=CH3 … 3=CH6), per operation mode
    BtnConfig btnDriving[4];
    BtnConfig btnStationary[4];
} ArtooConfig;

// --- Real-time state ---

typedef struct {
    int     throttleVal;
    int     steerVal;
    int     secondThrottleVal;
    int     secondSteerVal;
    int     domeSpeed;
    float   batteryVoltage;
    int     mode;           // 0=driving, 1=stationary
} ArtooStatus;

// --- Custom Action ---

typedef enum {
    ACTION_MOVE_DOME = 0,
    ACTION_PLAY_SOUND,
    ACTION_OPEN_ARM1,
    ACTION_CLOSE_ARM1,
    ACTION_OPEN_ARM2,
    ACTION_CLOSE_ARM2,
    ACTION_SET_VOLUME,
} ActionType;

typedef struct {
    int         id;
    char        name[32];
    ActionType  type;
    int         param1;     // speed (dome), track number (sound), or volume level
    int         param2;     // duration ms (dome)
} CustomAction;
