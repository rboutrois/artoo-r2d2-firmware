#pragma once

// =============================================================================
// PANELS — servos des panneaux du dôme (et holoprojecteurs)
// =============================================================================
// Pilote les servos du dôme via DEUX cartes PCA9685 (I2C) :
//   - servo 0..15  -> carte 0, adresse 0x40
//   - servo 16..31 -> carte 1, adresse 0x41
// Les deux cartes sont sur le bus I2C (PIN_I2C_SDA/PIN_I2C_SCL de config.h),
// qui monte au dôme par le slipring.
//
// Squelette à compléter :
//   - calibrer les positions fermé/ouvert de chaque servo (voir panel_test),
//   - appeler panels_init() dans setup() de main.cpp,
//   - brancher les actions/endpoints (actions.cpp, wifi_server.cpp) si besoin,
//   - persister la calibration en NVS via config.cpp (TODO ci-dessous).

#define PANEL_COUNT           32      // 2 x PCA9685 (16 voies chacune)
#define PANEL_PCA_ADDR_0      0x40
#define PANEL_PCA_ADDR_1      0x41
#define PANEL_PWM_FREQ        50      // Hz, fréquence standard des servos

// Valeurs PWM 12 bits (0..4095) par défaut — À CALIBRER servo par servo.
// Repère : à 50 Hz, ~150 ≈ 1 ms (mini) et ~600 ≈ 2 ms (maxi).
#define PANEL_DEFAULT_CLOSED  150
#define PANEL_DEFAULT_OPEN    600

void panels_init();                          // init I2C + les 2 cartes, tout fermer
void panel_set(int servo, bool open);        // ouvre/ferme un panneau (0..PANEL_COUNT-1)
void panels_all_close();                     // ferme tous les panneaux
void panel_test(int servo, int pwmCount);    // envoie une valeur brute 12 bits (calibration)

// Calibration par servo (en RAM pour l'instant).
// TODO: charger/sauver depuis ArtooConfig (NVS) via config.cpp.
void panel_set_calibration(int servo, int closedCount, int openCount);
int  panel_get_closed(int servo);
int  panel_get_open(int servo);
