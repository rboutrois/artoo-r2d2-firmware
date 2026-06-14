# Artoo R2D2 Controller — Spécification Fonctionnelle Complète

> Extraite par reverse engineering du binaire `Artoo-v1.1_Beta_Full.bin` + analyse des screenshots UI + posts Facebook de Steve Wagg.

---

## 1. Modes de fonctionnement

Deux modes basculables via bouton RC ou bouton dans l'UI :

| Mode | Stick gauche | Stick droit | Boutons |
|------|-------------|-------------|---------|
| **Driving** | Avance / Recule (throttle) | Direction gauche/droite (steer) | Actions programmables |
| **Stationary** | Rotation dôme | Volume | Sons + Bras |

---

## 2. Récepteur RC

Trois modes configurables :

| Mode | Description | Pins |
|------|-------------|------|
| Standard PWM | 6 canaux PWM classiques | GPIO 15, 13, 2, 4, 12, 27 |
| Single SBUS | 1 récepteur SBUS → 6 canaux | GPIO 15 (Serial2 RX) |
| **Dual SBUS** | 2 récepteurs SBUS → 12 canaux | GPIO 15 + GPIO 13 |

**SBUS** : 100000 baud, 8E2, signal **inversé**. Librairie : `bolderflight/SBUS`.  
Télécommandes utilisées : HOTRC 650 (6 canaux).

Mapping canaux (Driving mode) :
- Canal 1 → Throttle (avance/recule)
- Canal 2 → Steer (direction)
- Canaux 3–6 → Boutons programmables

Mapping canaux (Stationary mode) :
- Canal 1 → Rotation dôme
- Canal 2 → Volume
- Canaux 3–6 → Boutons programmables

---

## 3. Moteurs — Protocole Hoverserial

**Interface** : Serial1 (UART), **115200 baud**, câble USART3 du hoverboard (3.3V).  
**Fréquence d'envoi** : toutes les 100ms.

### Trame commande ESP32 → Hoverboard (8 bytes)
```c
typedef struct {
    uint16_t start;     // 0xABCD  (little-endian)
    int16_t  steer;     // -1000 à +1000  (direction)
    int16_t  speed;     // -1000 à +1000  (vitesse)
    uint16_t checksum;  // start ^ steer ^ speed
} SerialCommand;
```

### Trame feedback Hoverboard → ESP32 (18 bytes)
```c
typedef struct {
    uint16_t start;         // 0xABCD
    int16_t  cmd1;
    int16_t  cmd2;
    int16_t  speedR_meas;   // vitesse roue droite mesurée
    int16_t  speedL_meas;   // vitesse roue gauche mesurée
    int16_t  batVoltage;    // tension batterie × 100 (ex: 3600 = 36.00V)
    int16_t  boardTemp;
    uint16_t cmdLed;
    uint16_t checksum;      // XOR de tous les champs sauf checksum
} SerialFeedback;
```

Valeurs par défaut UI :
- Top Speed : **300** (range 50–1000)
- Top Steer : **155** (range configurable)
- Direction moteur gauche : configurable (toggle REVERSE)
- Direction moteur droit : configurable (toggle REVERSE)

---

## 4. Dôme

**Interface** : signal servo PWM via `ledcWrite` ou `ESP32Servo`.  
Connecteur DOME sur le PCB Artoo.

### Valeurs servo
| Paramètre | Valeur par défaut |
|-----------|-----------------|
| Stop (center) | 90 |
| Full Reverse | 50 |
| Full Forward | 130 |
| Dome Speed Range | 3 (slider 1–5) |

Correspondance niveaux :
- Level 1 = Gentle → range 70–110
- Level 5 = Maximum → range 10–170

### Lissage mouvement
| Paramètre | Valeur par défaut |
|-----------|-----------------|
| Acceleration | 15 /s² |
| Deceleration | 3 /s² |

### Rotation aléatoire
| Paramètre | Valeur par défaut |
|-----------|-----------------|
| Random Dome | OFF |
| Min Time | 10 s |
| Max Time | 60 s |

---

## 5. Son — DY-SV5W MP3 Player

**Interface** : UART série, **9600 baud**, port S2 du PCB Artoo.  
Sons stockés sur microSD dans le module DY-SV5W.

### Commandes DY-SV5W (protocole hex série)
```
Play track N  : AA 07 02 [HIGH_BYTE] [LOW_BYTE] [CHECKSUM]
Stop          : AA 04 00 AE
Set Volume N  : AA 13 01 [N] [CHECKSUM]  (N = 0 à 30)
```
Checksum = somme de tous les bytes du paquet (sans overflow).

### Paramètres
| Paramètre | Valeur par défaut |
|-----------|-----------------|
| Volume | 25 (range 0–30) |
| Startup Sound | configurable (numéro de son) |
| Random Sounds | OFF |
| Min Time | 4 s |
| Max Time | 40 s |
| Nombre de sons | variable (dépend de la microSD) — UI originale : 70 |

---

## 6. Bras — PCA9685

**Interface** : I2C, adresse **0x40**.  
Librairie : `Adafruit_PWMServoDriver`.  
Connecteurs ARM1 (top) et ARM2 (bottom) sur le PCB.

### Calibration par défaut (identique Arm1 et Arm2)
| Paramètre | Valeur |
|-----------|--------|
| Open Position | 60° |
| Closed Position | 110° |
| Min Pulse | 1000 µs |
| Max Pulse | 2000 µs |

États : `Closed` au démarrage.

---

## 7. Boutons programmables (canaux 3–6)

Actions assignables à chaque bouton via l'UI :
- `None`
- `Toggle Drive and Stationary Mode`
- `Play Random Sound`
- `Play Random Whistle`
- `Play Specific Sound [N]`
- `Move Dome [speed, duration]`
- `Open Arm 1` / `Close Arm 1`
- `Open Arm 2` / `Close Arm 2`
- `Custom Action [id]`
- `Sequence [id]`

---

## 8. Custom Actions

Actions atomiques programmables depuis l'UI, assignables aux boutons.

Types d'action disponibles :
- `Move Dome` (speed: -100 à 100, duration: ms)
- `Play Sound` (numéro)
- `Open Arm 1` / `Close Arm 1`
- `Open Arm 2` / `Close Arm 2`
- `Set Volume` (valeur)

Stockage : JSON dans SPIFFS (`/custom_actions.json`).

---

## 9. Séquences

Enchaînements d'actions avec délais.  
Stockage : JSON dans SPIFFS (`/sequences.json`).

---

## 10. API HTTP complète

Le WebServer écoute sur le port **80** en mode **Access Point WiFi**.

### Pages
| Route | Description |
|-------|-------------|
| `GET /` | Page principale (Home) |
| `GET /manual` | Page contrôle manuel |
| `GET /config` | Page configuration |

### Endpoints API
| Méthode | Route | Paramètres | Description |
|---------|-------|-----------|-------------|
| GET | `/status` | — | JSON état temps réel |
| GET | `/getConfig` | — | Config complète JSON |
| POST | `/saveConfig` | JSON body | Sauvegarde config |
| GET | `/getWiFiConfig` | — | Config WiFi |
| POST | `/setWiFiConfig` | ssid, password | Change WiFi AP |
| POST | `/setSpeed` | value (50–1000) | Vitesse max |
| POST | `/setSteer` | value | Limite braquage |
| POST | `/setMode` | value (0=driving, 1=stationary) | Mode opération |
| POST | `/setReceiverMode` | value (0=PWM, 1=SBUS, 2=DualSBUS) | Mode récepteur |
| POST | `/setLeftMotorDir` | value (0=forward, 1=reverse) | Direction moteur gauche |
| POST | `/setRightMotorDir` | value (0=forward, 1=reverse) | Direction moteur droit |
| POST | `/setDomeManual` | speed (-100 à 100) | Rotation manuelle dôme |
| POST | `/setDomeCalibration` | center, min, max | Calibration servo dôme |
| POST | `/setDomeSmoothing` | accel, decel | Lissage dôme |
| POST | `/setDomeTimerSettings` | min_time, max_time | Intervalle rotation auto |
| POST | `/setRandomDome` | value (0\|1) | Toggle rotation aléatoire |
| POST | `/setRandomSounds` | value (0\|1) | Toggle sons aléatoires |
| POST | `/setVolume` | value (0–30) | Volume |
| POST | `/setStartupSound` | value | Son de démarrage |
| POST | `/setTimerSettings` | min_time, max_time | Timer sons aléatoires |
| POST | `/playSound` | — | Joue son aléatoire |
| POST | `/playSpecificSound` | number (1–N) | Joue son numéro N |
| POST | `/setArm1Positions` | open_pos, closed_pos | Positions servo bras 1 |
| POST | `/setArm1Pulse` | min_pulse, max_pulse | Impulsions servo bras 1 |
| POST | `/setArm2Positions` | open_pos, closed_pos | Positions servo bras 2 |
| POST | `/setArm2Pulse` | min_pulse, max_pulse | Impulsions servo bras 2 |
| POST | `/testArm1` | position (open\|closed) | Test bras 1 |
| POST | `/testArm2` | position (open\|closed) | Test bras 2 |
| GET | `/getCustomActions` | — | Liste actions JSON |
| POST | `/saveCustomAction` | JSON | Sauvegarde action |
| POST | `/deleteCustomAction` | id | Supprime action |
| POST | `/testCustomAction` | id | Teste action |
| POST | `/exportCustomActions` | — | Export JSON |
| POST | `/importCustomActions` | JSON | Import JSON |
| GET | `/getSequences` | — | Liste séquences JSON |
| POST | `/saveSequence` | JSON | Sauvegarde séquence |
| POST | `/deleteSequence` | id | Supprime séquence |
| POST | `/testSequence` | id | Teste séquence |
| POST | `/exportSequences` | — | Export JSON |
| POST | `/importSequences` | JSON | Import JSON |
| POST | `/emergencyStop` | — | Arrêt d'urgence |
| POST | `/update` | binary | OTA firmware upload |

### JSON `/status` (exemple)
```json
{
  "speed": 300,
  "steer": 155,
  "dome_speed": 0,
  "dome_center": 90,
  "dome_min": 50,
  "dome_max": 130,
  "dome_accel": 15,
  "dome_decel": 3,
  "dome_min_time": 10,
  "dome_max_time": 60,
  "random_sounds": 0,
  "random_dome": 0,
  "left_motor_forward": 1,
  "right_motor_forward": 1,
  "startup_sound": 1,
  "steer_val": 0,
  "throttle_val": 0,
  "second_steer_val": 0,
  "second_throttle_val": 0,
  "battery": 0,
  "volume": 25,
  "mode": 0,
  "receiver_mode": 2
}
```

---

## 11. WiFi

- Mode : **Access Point (AP)**
- SSID par défaut : `ArtooR2D2` (à confirmer)
- Password : configurable
- SSID/Password sauvegardés en NVS (Preferences)
- Changement via `/setWiFiConfig`

---

## 12. OTA

- Endpoint : `POST /update` (multipart/form-data, fichier .bin)
- Librairie : `Update.h` (inclus dans Arduino-ESP32)
- Double partition app0/app1 + otadata → rollback possible

---

## 13. Persistance NVS

Toutes les valeurs configurables sont sauvegardées avec `Preferences` Arduino.  
Namespace suggéré : `"artoo"`.

Clés à persister : speed, steer, volume, startup_sound, random_sounds, random_dome, dome_center, dome_min, dome_max, dome_accel, dome_decel, dome_min_time, dome_max_time, left_motor_forward, right_motor_forward, receiver_mode, wifi_ssid, wifi_password, arm1_open, arm1_closed, arm1_min_pulse, arm1_max_pulse, arm2_open, arm2_closed, arm2_min_pulse, arm2_max_pulse.
