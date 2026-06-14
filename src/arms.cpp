#include <ESP32Servo.h>
#include "arms.h"
#include "config.h"

static Servo              s_arm1;
static Servo              s_arm2;
static const ArtooConfig* s_cfg = nullptr;

void arms_init(const ArtooConfig* cfg) {
    s_cfg = cfg;
    // Attach with calibrated pulse range from config
    s_arm1.attach(PIN_ARM1, cfg->arm1MinPulse, cfg->arm1MaxPulse);
    s_arm2.attach(PIN_ARM2, cfg->arm2MinPulse, cfg->arm2MaxPulse);
    // Park both arms closed at startup
    s_arm1.write(cfg->arm1ClosedPos);
    s_arm2.write(cfg->arm2ClosedPos);
}

void arms_set(int arm, bool open) {
    if (!s_cfg) return;
    if (arm == 1) s_arm1.write(open ? s_cfg->arm1OpenPos : s_cfg->arm1ClosedPos);
    if (arm == 2) s_arm2.write(open ? s_cfg->arm2OpenPos : s_cfg->arm2ClosedPos);
}
