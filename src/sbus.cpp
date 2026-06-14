#include <sbus.h>
#include "sbus_receiver.h"
#include "config.h"

// SBUS1: UART2 (Serial2), GPIO 15 RX, inverted signal
static bfs::SbusRx sbus1(&Serial2, PIN_SBUS1, -1, true);

// SBUS2 (Dual mode, GPIO 13) — TODO: all 3 ESP32 hardware UARTs are already
// allocated (UART0=Sound, UART1=Hoverboard, UART2=SBUS1). Requires either a
// software UART solution or hardware redesign confirmation before implementing.

static int  s_mode     = RECEIVER_SBUS;
static bool s_failsafe = true;

// Button edge detection for CH3..CH6 (index 0..3)
static bool s_btnState[4] = {false, false, false, false};
static bool s_btnEdge[4]  = {false, false, false, false};

// Map SBUS 11-bit value (typ. 172–1811, center 992) to signed -1000…+1000
static int sbusToSigned(int16_t raw) {
    return (int)map((long)raw, SBUS_MIN, SBUS_MAX, -1000, 1000);
}

void sbus_init(int mode) {
    s_mode = mode;
    if (mode == RECEIVER_SBUS || mode == RECEIVER_DUAL_SBUS) {
        sbus1.Begin();
    }
    // RECEIVER_PWM: pin-based pulse measurement — not yet implemented
}

void sbus_update(ArtooStatus* status) {
    if (s_mode == RECEIVER_PWM) return;

    if (!sbus1.Read()) return;

    const auto& d = sbus1.data();

    if (d.failsafe || d.lost_frame) {
        s_failsafe            = true;
        status->throttleVal   = 0;
        status->steerVal      = 0;
        // Zero secondary channels too so hoverboard sees a safe stop
        status->secondThrottleVal = 0;
        status->secondSteerVal    = 0;
        return;
    }

    s_failsafe          = false;
    status->throttleVal = sbusToSigned(d.ch[CH_THROTTLE]);
    status->steerVal    = sbusToSigned(d.ch[CH_STEER]);

    // Detect rising edges on CH3..CH6 (button channels)
    for (int i = 0; i < 4; i++) {
        bool high = (d.ch[CH_BTN3 + i] > 1400);
        if (high && !s_btnState[i]) s_btnEdge[i] = true;
        s_btnState[i] = high;
    }
}

bool sbus_button_just_pressed(int ch) {
    if (ch < 0 || ch >= 4) return false;
    bool edge = s_btnEdge[ch];
    s_btnEdge[ch] = false;
    return edge;
}

bool sbus_button_state(int ch) {
    if (ch < 0 || ch >= 4) return false;
    return s_btnState[ch];
}

bool sbus_failsafe() {
    return s_failsafe;
}
