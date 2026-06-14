#include <Arduino.h>
#include "sound.h"
#include "config.h"

// DY-SV5W communicates at 9600 baud TX-only.
// All 3 ESP32 hardware UARTs are allocated (SBUS/Hoverboard/Sound is last).
// We use bit-bang TX on PIN_SOUND_TX — at 9600 baud each byte takes ~1.04 ms,
// so a 6-byte command blocks for ~6 ms. Acceptable given sounds are infrequent.

static const uint32_t BIT_US = 1000000UL / SOUND_BAUD;   // ≈ 104 µs per bit

static int  s_trackCount   = 70;   // default from spec; update if known
static unsigned long s_nextRandom = 0;

// FreeRTOS critical-section lock — prevents WiFi ISR from preempting between
// bits and corrupting the bit-bang timing.
static portMUX_TYPE s_tx_mux = portMUX_INITIALIZER_UNLOCKED;

// ---------------------------------------------------------------------------
// Low-level bit-bang UART TX (8N1, not inverted)
// ---------------------------------------------------------------------------

static void txByte(uint8_t b) {
    taskENTER_CRITICAL(&s_tx_mux);
    // START bit
    digitalWrite(PIN_SOUND_TX, LOW);
    delayMicroseconds(BIT_US);
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_SOUND_TX, (b >> i) & 1 ? HIGH : LOW);
        delayMicroseconds(BIT_US);
    }
    // STOP bit
    digitalWrite(PIN_SOUND_TX, HIGH);
    delayMicroseconds(BIT_US);
    taskEXIT_CRITICAL(&s_tx_mux);
}

static void txPacket(const uint8_t* buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) txByte(buf[i]);
}

// ---------------------------------------------------------------------------
// DY-SV5W protocol helpers
// Checksum = sum of all bytes in the packet (lower 8 bits)
// ---------------------------------------------------------------------------

static void cmdPlayTrack(int n) {
    uint8_t hi = (uint8_t)((n >> 8) & 0xFF);
    uint8_t lo = (uint8_t)(n & 0xFF);
    uint8_t buf[] = { 0xAA, 0x07, 0x02, hi, lo,
                      (uint8_t)(0xAA + 0x07 + 0x02 + hi + lo) };
    txPacket(buf, sizeof(buf));
}

static void cmdStop() {
    uint8_t buf[] = { 0xAA, 0x04, 0x00, 0xAE };
    txPacket(buf, sizeof(buf));
}

static void cmdSetVolume(int v) {
    uint8_t vol = (uint8_t)v;
    uint8_t buf[] = { 0xAA, 0x13, 0x01, vol,
                      (uint8_t)(0xAA + 0x13 + 0x01 + vol) };
    txPacket(buf, sizeof(buf));
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void sound_init(const ArtooConfig* cfg) {
    pinMode(PIN_SOUND_TX, OUTPUT);
    digitalWrite(PIN_SOUND_TX, HIGH);   // idle HIGH for UART
    delay(600);                          // let DY-SV5W finish booting

    cmdSetVolume(cfg->volume);

    if (cfg->startupSound > 0) {
        delay(200);
        cmdPlayTrack(cfg->startupSound);
    }

    // Schedule first random sound after the configured minimum interval
    s_nextRandom = millis() + (unsigned long)cfg->soundMinTime * 1000UL;
}

void sound_set_volume(int volume) {
    if (volume < 0)  volume = 0;
    if (volume > 30) volume = 30;
    cmdSetVolume(volume);
}

void sound_play(int track) {
    if (track < 1) track = 1;
    cmdPlayTrack(track);
}

void sound_play_random() {
    if (s_trackCount < 1) return;
    cmdPlayTrack(random(1, s_trackCount + 1));
}

void sound_stop() {
    cmdStop();
}

void sound_update(const ArtooConfig* cfg, ArtooStatus* status) {
    // Random sounds timer
    if (!cfg->randomSounds) return;
    unsigned long now = millis();
    if ((long)(now - s_nextRandom) < 0) return;

    sound_play_random();
    unsigned long interval = (unsigned long)random(
        cfg->soundMinTime * 1000,
        cfg->soundMaxTime * 1000 + 1);
    s_nextRandom = now + interval;
}
