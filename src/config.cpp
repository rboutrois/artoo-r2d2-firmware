#include <Preferences.h>
#include "config_mgr.h"
#include "config.h"

static Preferences prefs;

void config_init(ArtooConfig* cfg) {
    prefs.begin(NVS_NAMESPACE, false);

    cfg->speed          = prefs.getInt("speed",      DEFAULT_SPEED);
    cfg->steer          = prefs.getInt("steer",      DEFAULT_STEER);
    cfg->leftMotorFwd   = prefs.getBool("lmfwd",     DEFAULT_MOTOR_L_FWD);
    cfg->rightMotorFwd  = prefs.getBool("rmfwd",     DEFAULT_MOTOR_R_FWD);

    cfg->domeCenter     = prefs.getInt("dome_ctr",   DEFAULT_DOME_CENTER);
    cfg->domeMin        = prefs.getInt("dome_min",   DEFAULT_DOME_MIN);
    cfg->domeMax        = prefs.getInt("dome_max",   DEFAULT_DOME_MAX);
    cfg->domeSpeedRange = prefs.getInt("dome_spd",   DEFAULT_DOME_SPEED_RANGE);
    cfg->domeAccel      = prefs.getInt("dome_acc",   DEFAULT_DOME_ACCEL);
    cfg->domeDecel      = prefs.getInt("dome_dec",   DEFAULT_DOME_DECEL);
    cfg->randomDome     = prefs.getBool("rnd_dome",  DEFAULT_DOME_RANDOM);
    cfg->domeMinTime    = prefs.getInt("dome_tmin",  DEFAULT_DOME_MIN_TIME);
    cfg->domeMaxTime    = prefs.getInt("dome_tmax",  DEFAULT_DOME_MAX_TIME);

    cfg->volume         = prefs.getInt("volume",     DEFAULT_VOLUME);
    cfg->startupSound   = prefs.getInt("snd_start",  DEFAULT_STARTUP_SOUND);
    cfg->randomSounds   = prefs.getBool("rnd_snd",   DEFAULT_RANDOM_SOUNDS);
    cfg->soundMinTime   = prefs.getInt("snd_tmin",   DEFAULT_SOUND_MIN_TIME);
    cfg->soundMaxTime   = prefs.getInt("snd_tmax",   DEFAULT_SOUND_MAX_TIME);

    cfg->arm1OpenPos    = prefs.getInt("a1_open",    DEFAULT_ARM1_OPEN_POS);
    cfg->arm1ClosedPos  = prefs.getInt("a1_cls",     DEFAULT_ARM1_CLOSED_POS);
    cfg->arm1MinPulse   = prefs.getInt("a1_pmin",    DEFAULT_ARM1_MIN_PULSE);
    cfg->arm1MaxPulse   = prefs.getInt("a1_pmax",    DEFAULT_ARM1_MAX_PULSE);

    cfg->arm2OpenPos    = prefs.getInt("a2_open",    DEFAULT_ARM2_OPEN_POS);
    cfg->arm2ClosedPos  = prefs.getInt("a2_cls",     DEFAULT_ARM2_CLOSED_POS);
    cfg->arm2MinPulse   = prefs.getInt("a2_pmin",    DEFAULT_ARM2_MIN_PULSE);
    cfg->arm2MaxPulse   = prefs.getInt("a2_pmax",    DEFAULT_ARM2_MAX_PULSE);

    cfg->receiverMode   = prefs.getInt("rcv_mode",   RECEIVER_SBUS);

    cfg->crowdLimit     = prefs.getBool("crowd_on",  DEFAULT_CROWD_LIMIT);
    cfg->crowdSpeed     = prefs.getInt("crowd_spd",  DEFAULT_CROWD_SPEED);

    cfg->greeterEnabled   = prefs.getBool("greet_on",  DEFAULT_GREETER_ENABLED);
    cfg->greeterIntensity = prefs.getInt("greet_lvl",  DEFAULT_GREETER_INTENSITY);
    cfg->trackCount       = prefs.getInt("trk_count",  DEFAULT_TRACK_COUNT);
    cfg->trackOffset      = prefs.getInt("trk_off",    DEFAULT_TRACK_OFFSET);

    for (int i = 0; i < 4; i++) {
        char ka[8], kp[9];
        snprintf(ka, sizeof(ka), "bd%d_a",  i);
        snprintf(kp, sizeof(kp), "bd%d_p1", i);
        cfg->btnDriving[i].action = prefs.getUChar(ka, BTN_NONE);
        cfg->btnDriving[i].param1 = prefs.getShort(kp, 0);
        snprintf(ka, sizeof(ka), "bs%d_a",  i);
        snprintf(kp, sizeof(kp), "bs%d_p1", i);
        cfg->btnStationary[i].action = prefs.getUChar(ka, BTN_NONE);
        cfg->btnStationary[i].param1 = prefs.getShort(kp, 0);
    }

    String ssid = prefs.getString("wifi_ssid", DEFAULT_WIFI_SSID);
    String pass = prefs.getString("wifi_pass",  DEFAULT_WIFI_PASSWORD);
    strlcpy(cfg->wifiSSID,     ssid.c_str(), sizeof(cfg->wifiSSID));
    strlcpy(cfg->wifiPassword, pass.c_str(), sizeof(cfg->wifiPassword));

    prefs.end();
}

void config_save(const ArtooConfig* cfg) {
    prefs.begin(NVS_NAMESPACE, false);

    prefs.putInt("speed",      cfg->speed);
    prefs.putInt("steer",      cfg->steer);
    prefs.putBool("lmfwd",     cfg->leftMotorFwd);
    prefs.putBool("rmfwd",     cfg->rightMotorFwd);

    prefs.putInt("dome_ctr",   cfg->domeCenter);
    prefs.putInt("dome_min",   cfg->domeMin);
    prefs.putInt("dome_max",   cfg->domeMax);
    prefs.putInt("dome_spd",   cfg->domeSpeedRange);
    prefs.putInt("dome_acc",   cfg->domeAccel);
    prefs.putInt("dome_dec",   cfg->domeDecel);
    prefs.putBool("rnd_dome",  cfg->randomDome);
    prefs.putInt("dome_tmin",  cfg->domeMinTime);
    prefs.putInt("dome_tmax",  cfg->domeMaxTime);

    prefs.putInt("volume",     cfg->volume);
    prefs.putInt("snd_start",  cfg->startupSound);
    prefs.putBool("rnd_snd",   cfg->randomSounds);
    prefs.putInt("snd_tmin",   cfg->soundMinTime);
    prefs.putInt("snd_tmax",   cfg->soundMaxTime);

    prefs.putInt("a1_open",    cfg->arm1OpenPos);
    prefs.putInt("a1_cls",     cfg->arm1ClosedPos);
    prefs.putInt("a1_pmin",    cfg->arm1MinPulse);
    prefs.putInt("a1_pmax",    cfg->arm1MaxPulse);

    prefs.putInt("a2_open",    cfg->arm2OpenPos);
    prefs.putInt("a2_cls",     cfg->arm2ClosedPos);
    prefs.putInt("a2_pmin",    cfg->arm2MinPulse);
    prefs.putInt("a2_pmax",    cfg->arm2MaxPulse);

    prefs.putInt("rcv_mode",   cfg->receiverMode);

    prefs.putBool("crowd_on",  cfg->crowdLimit);
    prefs.putInt("crowd_spd",  cfg->crowdSpeed);

    prefs.putBool("greet_on",  cfg->greeterEnabled);
    prefs.putInt("greet_lvl",  cfg->greeterIntensity);
    prefs.putInt("trk_count",  cfg->trackCount);
    prefs.putInt("trk_off",    cfg->trackOffset);
    for (int i = 0; i < 4; i++) {
        char ka[8], kp[9];
        snprintf(ka, sizeof(ka), "bd%d_a",  i);
        snprintf(kp, sizeof(kp), "bd%d_p1", i);
        prefs.putUChar(ka, cfg->btnDriving[i].action);
        prefs.putShort(kp, cfg->btnDriving[i].param1);
        snprintf(ka, sizeof(ka), "bs%d_a",  i);
        snprintf(kp, sizeof(kp), "bs%d_p1", i);
        prefs.putUChar(ka, cfg->btnStationary[i].action);
        prefs.putShort(kp, cfg->btnStationary[i].param1);
    }

    prefs.putString("wifi_ssid", cfg->wifiSSID);
    prefs.putString("wifi_pass", cfg->wifiPassword);

    prefs.end();
}

void config_reset() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
}
