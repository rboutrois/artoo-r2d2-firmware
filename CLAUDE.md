# Artoo R2D2 Controller — Firmware

## Contexte du projet

Reconstruction from scratch du firmware de la carte **Artoo R2D2 Controller v1.1** par Steve Wagg.
Le firmware original (binaire seul, pas de source disponible) a été analysé par reverse engineering statique.
Toutes les specs fonctionnelles sont dans `../spec.md`.

## Matériel cible

- **MCU** : ESP32 D1 Mini
- **Framework** : Arduino (via PlatformIO)
- **Arduino-ESP32** : 3.3.x
- **Flasher** : PlatformIO ou OTA via `/update`

## Architecture du projet

```
firmware/
├── src/
│   ├── main.cpp          ← point d'entrée, setup/loop
│   ├── wifi_server.cpp   ← WiFi AP + WebServer HTTP
│   ├── sbus.cpp          ← lecture SBUS (Serial2)
│   ├── hoverboard.cpp    ← protocole Hoverserial (Serial1)
│   ├── dome.cpp          ← contrôle ESC dôme via PWM
│   ├── sound.cpp         ← DY-SV5W via Serial (Serial0 ou Serial2)
│   ├── arms.cpp          ← PCA9685 I2C → Arm1 + Arm2
│   ├── actions.cpp       ← custom actions + séquences
│   └── config.cpp        ← persistance NVS (Preferences)
├── include/
│   ├── config.h          ← toutes les constantes et valeurs par défaut
│   ├── types.h           ← structs partagées
│   └── *.h               ← headers des modules
├── data/                 ← fichiers SPIFFS (HTML/CSS/JS de l'UI web)
├── platformio.ini
├── CLAUDE.md             ← CE FICHIER
└── spec.md               ← specs complètes à lire avant de coder
```

## Règles de développement

- **Toujours lire `spec.md` avant de coder un module.** Toutes les valeurs par défaut, pins, et protocoles y sont.
- Un module = un .cpp + un .h. Pas de logique métier dans main.cpp.
- `main.cpp` ne contient que `setup()` et `loop()` + les appels aux modules.
- Les paramètres persistants passent **tous** par `config.cpp` (Preferences NVS). Jamais d'écriture directe dans les autres modules.
- Les endpoints HTTP sont déclarés dans `wifi_server.cpp`. Les handlers appellent les fonctions des modules, pas l'inverse.
- Tous les délais non-bloquants : utiliser `millis()`, jamais `delay()` dans la loop principale.
- Le WebServer tourne en tâche FreeRTOS séparée si nécessaire pour éviter les blocages.

## Ordre de développement recommandé

1. `config.h` + `config.cpp` — constantes et Preferences NVS
2. `wifi_server.cpp` — AP WiFi + endpoint `/status` (JSON vide d'abord)
3. `sbus.cpp` — lecture SBUS sur Serial2 pin 15
4. `hoverboard.cpp` — Hoverserial 115200 baud sur Serial1
5. `dome.cpp` — PWM LEDC, logique accel/decel
6. `sound.cpp` — DY-SV5W UART
7. `arms.cpp` — PCA9685 I2C
8. `actions.cpp` — custom actions + séquences JSON SPIFFS
9. Tous les endpoints HTTP de `spec.md`
10. OTA `/update`

## Pins ESP32 D1 Mini (mappés depuis le PCB Artoo)

| Fonction | Pin ESP32 | Serial/Interface |
|----------|-----------|-----------------|
| SBUS1 (récepteur 1) | GPIO 15 | Serial2 RX |
| SBUS2 (récepteur 2) | GPIO 13 | Serial (configurable) |
| Hoverboard UART TX | GPIO 17 (TXD) | Serial1 TX |
| Hoverboard UART RX | GPIO 16 (RXD) | Serial1 RX |
| Sound DY-SV5W TX | dédié S2 PCB | Serial TX |
| Sound DY-SV5W RX | dédié S2 PCB | Serial RX |
| Dome ESC PWM | connecteur DOME PCB | ledcWrite |
| Arm1 servo | PCA9685 ch0 | I2C |
| Arm2 servo | PCA9685 ch1 | I2C |
| I2C SDA | GPIO 21 | Wire |
| I2C SCL | GPIO 22 | Wire |

> Note : les pins S1/S2/S3 du PCB Artoo correspondent aux UARTs hardware de l'ESP32.
> Confirmer le mapping exact avec un multimètre si besoin.

## Points d'attention

- **Hoverboard** : protocole Hoverserial, START_FRAME = 0xABCD, checksum = XOR de tous les champs. Voir `spec.md`.
- **DY-SV5W** : commandes hex série, baud 9600. Différent du DFPlayer Mini — ne pas utiliser la lib DFPlayer.
- **SBUS** : signal **inversé** (UART inversé 100000 baud, 8E2). Utiliser la lib `bolderflight/SBUS` qui gère ça.
- **Dome ESC** : valeurs servo (center=90, min=50, max=130 en degrés mappés). Utiliser `ESP32Servo` ou `ledcWrite`.
- **PCA9685** : adresse I2C par défaut 0x40. Utiliser `Adafruit_PWMServoDriver`.
- **SPIFFS** : l'UI web (HTML/CSS/JS) sera dans `/data`. Uploader séparément avec `pio run --target uploadfs`.
