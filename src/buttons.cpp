#include "buttons.h"
#include "sbus_receiver.h"
#include "dome.h"
#include "sound.h"
#include "arms.h"
#include "actions.h"
#include "config.h"

static void execute(const BtnConfig& b, ArtooConfig* cfg, ArtooStatus* status) {
    switch (b.action) {
        case BTN_TOGGLE_MODE:
            status->mode = (status->mode == MODE_DRIVING) ? MODE_STATIONARY : MODE_DRIVING;
            break;
        case BTN_PLAY_RANDOM:   sound_play_random();          break;
        case BTN_PLAY_SOUND:    sound_play(b.param1);         break;
        case BTN_OPEN_ARM1:     arms_set(1, true);            break;
        case BTN_CLOSE_ARM1:    arms_set(1, false);           break;
        case BTN_OPEN_ARM2:     arms_set(2, true);            break;
        case BTN_CLOSE_ARM2:    arms_set(2, false);           break;
        case BTN_MOVE_DOME:     dome_set_speed(b.param1);     break;
        case BTN_CUSTOM_ACTION: action_run(b.param1, cfg);    break;
        case BTN_SEQUENCE:      sequence_run(b.param1, cfg);  break;
        default: break;
    }
}

void buttons_update(ArtooConfig* cfg, ArtooStatus* status) {
    BtnConfig* btns = (status->mode == MODE_DRIVING)
        ? cfg->btnDriving
        : cfg->btnStationary;
    for (int i = 0; i < 4; i++) {
        if (sbus_button_just_pressed(i)) {
            execute(btns[i], cfg, status);
        }
    }
}
