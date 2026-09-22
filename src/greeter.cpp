#include <Arduino.h>
#include "greeter.h"
#include "config.h"
#include "dome.h"
#include "sound.h"

// ---------------------------------------------------------------------------
// Scene description
// ---------------------------------------------------------------------------
// A scene is a short list of steps played without ever blocking loop().
// Every step waits `waitMs` before the next one runs.

typedef enum {
    STEP_DOME = 0,      // p1 = speed -100..100, p2 = move duration (ms)
    STEP_TRACK,         // p1 = track number
    STEP_TRACK_RANGE,   // p1..p2 = pick one track at random in this range
    STEP_PAUSE,         // nothing, just wait
} StepType;

typedef struct {
    uint8_t type;
    int16_t p1;
    int16_t p2;
    int16_t waitMs;
} Step;

typedef struct {
    const char* name;
    bool        ambient;     // may be picked by the idle loop
    const Step* steps;
    uint8_t     stepCount;
} Scene;

// --- Ambient scenes (short, quiet enough to run every few seconds) ----------

static const Step SC_GLANCE[] = {
    { STEP_DOME,        40,  600,  500 },
    { STEP_TRACK_RANGE, TRACK_OOH_FIRST, TRACK_OOH_LAST, 900 },
    { STEP_DOME,       -25,  400,  400 },
};

static const Step SC_CURIOUS[] = {
    { STEP_DOME,       -35, 900,  1000 },
    { STEP_TRACK_RANGE, TRACK_OOH_FIRST, TRACK_OOH_LAST, 1200 },
    { STEP_DOME,        20, 500,   500 },
};

static const Step SC_CHATTER[] = {
    { STEP_TRACK_RANGE, TRACK_SENT_FIRST, TRACK_SENT_LAST, 2600 },
    { STEP_DOME,        30, 400,   600 },
    { STEP_TRACK_RANGE, TRACK_SENT_FIRST, TRACK_SENT_LAST, 2600 },
};

static const Step SC_GRUMBLE[] = {
    { STEP_TRACK_RANGE, TRACK_HUM_FIRST, TRACK_HUM_LAST, 900 },
    { STEP_DOME,       -20, 300,   350 },
    { STEP_DOME,        20, 300,   350 },
    { STEP_TRACK_RANGE, TRACK_HUM_FIRST, TRACK_HUM_LAST, 900 },
};

static const Step SC_BEEP[] = {
    { STEP_TRACK_RANGE, TRACK_MISC_FIRST, TRACK_MISC_LAST, 1500 },
};

static const Step SC_LOOKAROUND[] = {
    { STEP_DOME,        30, 1200, 1400 },
    { STEP_PAUSE,        0,    0,  800 },
    { STEP_DOME,       -30, 1400, 1600 },
};

// --- Manual scenes (only on request) ----------------------------------------

static const Step SC_HELLO[] = {
    { STEP_TRACK,       TRACK_WHISTLE, 0, 900 },
    { STEP_DOME,        45, 700,  800 },
    { STEP_DOME,       -35, 600,  600 },
};

static const Step SC_ALERT[] = {
    { STEP_TRACK_RANGE, TRACK_ALARM_FIRST, TRACK_ALARM_LAST, 400 },
    { STEP_DOME,        70, 900, 1000 },
    { STEP_DOME,       -70, 900, 1000 },
};

static const Step SC_SCREAM[]  = { { STEP_TRACK, TRACK_SCREAM,  0, 2500 } };
static const Step SC_LAUGH[]   = { { STEP_TRACK, TRACK_CHORTLE, 0, 2800 } };
static const Step SC_ANNOYED[] = { { STEP_TRACK, TRACK_ANNOYED, 0, 3500 } };
static const Step SC_LEIA[]    = { { STEP_TRACK, TRACK_LEIA,    0, 100  } };
static const Step SC_THEME[]   = { { STEP_TRACK, TRACK_THEME,   0, 100  } };

#define SCENE(n, amb, arr) { n, amb, arr, (uint8_t)(sizeof(arr) / sizeof(Step)) }

static const Scene s_scenes[] = {
    SCENE("Coup d'oeil",  true,  SC_GLANCE),
    SCENE("Curieux",      true,  SC_CURIOUS),
    SCENE("Bavardage",    true,  SC_CHATTER),
    SCENE("Ronchon",      true,  SC_GRUMBLE),
    SCENE("Bip",          true,  SC_BEEP),
    SCENE("Tour d'horizon", true, SC_LOOKAROUND),
    SCENE("Bonjour",      false, SC_HELLO),
    SCENE("Alerte",       false, SC_ALERT),
    SCENE("Cri",          false, SC_SCREAM),
    SCENE("Rire",         false, SC_LAUGH),
    SCENE("Rale",         false, SC_ANNOYED),
    SCENE("Message Leia", false, SC_LEIA),
    SCENE("Theme",        false, SC_THEME),
};

static const int SCENE_COUNT = sizeof(s_scenes) / sizeof(Scene);

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static bool          s_enabled      = false;
static int           s_sceneIdx     = -1;   // scene being played, -1 = none
static int           s_stepIdx      = 0;
static unsigned long s_nextStepMs   = 0;
static unsigned long s_nextSceneMs  = 0;
static unsigned long s_suspendUntil = 0;   // operator took over

// Idle interval per intensity level, in ms
static const struct { unsigned long lo, hi; int domeScale; } s_levels[] = {
    { 12000, 25000,  60 },   // 0 — calm
    {  6000, 14000, 100 },   // 1 — normal
    {  3000,  7000, 130 },   // 2 — lively
};

static int levelOf(const ArtooConfig* cfg) {
    int l = cfg->greeterIntensity;
    if (l < 0) l = 0;
    if (l > 2) l = 2;
    return l;
}

// ---------------------------------------------------------------------------
// Step execution
// ---------------------------------------------------------------------------

// Every track number goes through the offset, so the whole bank can be shifted
// from the phone if the module numbers the SD card differently than expected.
static void playTrack(const ArtooConfig* cfg, int track) {
    sound_play(track + cfg->trackOffset);
}

static void runStep(const ArtooConfig* cfg, const Step& st) {
    switch (st.type) {
        case STEP_DOME: {
            int scale = s_levels[levelOf(cfg)].domeScale;
            dome_set_speed_for((st.p1 * scale) / 100, st.p2);
            break;
        }
        case STEP_TRACK:
            playTrack(cfg, st.p1);
            break;
        case STEP_TRACK_RANGE:
            playTrack(cfg, (int)random(st.p1, st.p2 + 1));
            break;
        case STEP_PAUSE:
        default:
            break;
    }
}

static void scheduleNextScene(const ArtooConfig* cfg) {
    const auto& lv = s_levels[levelOf(cfg)];
    s_nextSceneMs = millis() + (unsigned long)random(lv.lo, lv.hi + 1);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void greeter_init(const ArtooConfig* cfg) {
    s_enabled = cfg->greeterEnabled;
    scheduleNextScene(cfg);
}

void greeter_set_enabled(bool on) {
    s_enabled = on;
    if (!on) greeter_stop();
}

bool greeter_enabled() {
    return s_enabled;
}

bool greeter_play_scene(int id) {
    if (id < 0 || id >= SCENE_COUNT) return false;
    s_sceneIdx   = id;
    s_stepIdx    = 0;
    s_nextStepMs = millis();
    return true;
}

void greeter_stop() {
    s_sceneIdx = -1;
    dome_stop();
}

int greeter_scene_count() {
    return SCENE_COUNT;
}

const char* greeter_scene_name(int id) {
    return (id >= 0 && id < SCENE_COUNT) ? s_scenes[id].name : "";
}

bool greeter_scene_is_ambient(int id) {
    return (id >= 0 && id < SCENE_COUNT) ? s_scenes[id].ambient : false;
}

bool greeter_scene_running() {
    return s_sceneIdx >= 0;
}

void greeter_update(const ArtooConfig* cfg, ArtooStatus* status) {
    unsigned long now = millis();

    // Emergency stop wins over everything
    if (status->estop) {
        s_sceneIdx           = -1;
        status->greeterActive = false;
        return;
    }

    // The operator moving a stick always takes priority: drop the scene and
    // stay out of the way for a while before coming back to life.
    if (abs(status->throttleVal) > GREETER_STICK_DEADBAND ||
        abs(status->steerVal)    > GREETER_STICK_DEADBAND) {
        if (s_sceneIdx >= 0) {
            s_sceneIdx = -1;
            dome_stop();
        }
        s_suspendUntil = now + GREETER_TAKEOVER_MS;
    }

    bool suspended = (long)(now - s_suspendUntil) < 0;

    // Tells dome.cpp that the greeter owns the dome right now, so stick-driven
    // dome control in stationary mode does not fight the scene.
    status->greeterActive = s_enabled && !suspended;

    // Play the scene in progress
    if (s_sceneIdx >= 0) {
        const Scene& sc = s_scenes[s_sceneIdx];
        while (s_stepIdx < sc.stepCount && (long)(now - s_nextStepMs) >= 0) {
            const Step& st = sc.steps[s_stepIdx];
            s_stepIdx++;
            runStep(cfg, st);
            s_nextStepMs = now + (unsigned long)st.waitMs;
        }
        if (s_stepIdx >= sc.stepCount && (long)(now - s_nextStepMs) >= 0) {
            s_sceneIdx = -1;
            scheduleNextScene(cfg);
        }
        return;
    }

    // Idle: pick the next ambient scene when its time comes
    if (!s_enabled || suspended) return;

    if ((long)(now - s_nextSceneMs) >= 0) {
        int candidates[SCENE_COUNT];
        int n = 0;
        for (int i = 0; i < SCENE_COUNT; i++) {
            if (s_scenes[i].ambient) candidates[n++] = i;
        }
        if (n > 0) greeter_play_scene(candidates[random(0, n)]);
        else       scheduleNextScene(cfg);
    }
}
