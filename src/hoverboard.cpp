#include <Arduino.h>
#include <string.h>
#include "hoverboard.h"
#include "config.h"

// Persistent receive state
static uint8_t  s_rxBuf[sizeof(HoverFeedback)];
static uint8_t  s_rxIdx    = 0;
static uint8_t  s_prevByte = 0;

static HoverFeedback s_feedback  = {};
static unsigned long s_lastSend  = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Scale -1000…+1000 input to -limit…+limit; zero within deadband
static int16_t scale(int val, int limit) {
    if (abs(val) < 30) return 0;
    return (int16_t)((long)val * limit / 1000);
}

static void sendCommand(int16_t speed, int16_t steer) {
    HoverCommand cmd;
    cmd.start    = HOVER_START_FRAME;
    cmd.steer    = steer;
    cmd.speed    = speed;
    cmd.checksum = (uint16_t)(cmd.start ^ (uint16_t)cmd.steer ^ (uint16_t)cmd.speed);
    Serial1.write((const uint8_t*)&cmd, sizeof(cmd));
}

// ---------------------------------------------------------------------------
// Feedback receive — non-blocking byte-by-byte (ref: EFeru/hoverserial.ino)
// ---------------------------------------------------------------------------

static void receiveBytes(ArtooStatus* status) {
    while (Serial1.available()) {
        uint8_t  b          = (uint8_t)Serial1.read();
        uint16_t startFrame = ((uint16_t)b << 8) | s_prevByte;

        if (startFrame == HOVER_START_FRAME) {
            s_rxBuf[0] = s_prevByte;
            s_rxBuf[1] = b;
            s_rxIdx    = 2;
        } else if (s_rxIdx >= 2 && s_rxIdx < sizeof(HoverFeedback)) {
            s_rxBuf[s_rxIdx++] = b;
        }

        if (s_rxIdx == sizeof(HoverFeedback)) {
            HoverFeedback fb;
            memcpy(&fb, s_rxBuf, sizeof(fb));

            uint16_t cs = (uint16_t)(fb.start ^ (uint16_t)fb.cmd1 ^ (uint16_t)fb.cmd2
                ^ (uint16_t)fb.speedR_meas ^ (uint16_t)fb.speedL_meas
                ^ (uint16_t)fb.batVoltage  ^ (uint16_t)fb.boardTemp ^ fb.cmdLed);

            if (fb.start == HOVER_START_FRAME && cs == fb.checksum) {
                memcpy(&s_feedback, &fb, sizeof(s_feedback));
                // batVoltage is ×100 (e.g. 3600 = 36.00 V)
                status->batteryVoltage = s_feedback.batVoltage / 100.0f;
            }

            s_rxIdx = 0;
        }

        s_prevByte = b;
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void hoverboard_init() {
    // Drive TX HIGH before UART takes over — prevents the hoverboard from seeing
    // a spurious break/start condition on the floating line during ESP32 boot.
    pinMode(PIN_HOVER_TX, OUTPUT);
    digitalWrite(PIN_HOVER_TX, HIGH);
    delayMicroseconds(500);
    Serial1.begin(HOVER_SERIAL_BAUD, SERIAL_8N1, PIN_HOVER_RX, PIN_HOVER_TX);
}

void hoverboard_update(const ArtooConfig* cfg, ArtooStatus* status) {
    receiveBytes(status);

    unsigned long now = millis();
    if (now - s_lastSend < HOVER_SEND_INTERVAL) return;

    // Latched emergency stop or stationary mode: stop frames only, no drive
    if (status->estop || status->mode == MODE_STATIONARY) {
        s_lastSend = now;
        sendCommand(0, 0);
        return;
    }
    s_lastSend = now;

    // Crowd limit caps the configured top speed without touching it, so the
    // event setting can be switched on and off in one tap.
    int topSpeed = cfg->speed;
    if (cfg->crowdLimit && cfg->crowdSpeed < topSpeed) topSpeed = cfg->crowdSpeed;

    int16_t speed = scale(status->throttleVal, topSpeed);
    int16_t steer = scale(status->steerVal,    cfg->steer);

    // Apply motor direction flags:
    // Both reversed  → negate speed (vehicle goes backward when stick is forward)
    // One reversed   → negate steer (left/right differential is mirrored)
    if (!cfg->leftMotorFwd && !cfg->rightMotorFwd) {
        speed = -speed;
        steer = -steer;   // reversed wiring mirrors the steer differential too
    } else if (cfg->leftMotorFwd != cfg->rightMotorFwd) {
        steer = -steer;
    }

    sendCommand(speed, steer);
}

void hoverboard_send_stop() {
    sendCommand(0, 0);
}
