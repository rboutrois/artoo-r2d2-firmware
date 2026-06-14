# Artoo R2-D2 Firmware

Open, independent firmware for the **Artoo R2-D2 controller board** designed by **Steve Wagg**, running on an ESP32 (D1 Mini).

> **Status: work in progress.** This is a clean, independent reimplementation, not Steve's original firmware (which was never open-sourced). Please read [CREDITS.md](CREDITS.md) before forking or redistributing.

## Features

- Hoverboard motor drive for the feet (Hoverserial protocol, 36 V)
- Dome rotation (DC motor + ESC, smoothed accel/decel, random mode)
- Sound (DY-SV5W MP3 player)
- Utility arms
- Dome panel servos via two PCA9685 boards (I2C `0x40` / `0x41`) — in progress
- RC input: SBUS (single or dual receiver) or standard PWM (tested with a HOTRC 650)
- WiFi Access Point + embedded web UI for configuration, manual control, custom actions/sequences, and OTA updates

## Hardware

- Controller board: **Artoo Controller v1.1** (designed by Steve Wagg)
- MCU: ESP32 D1 Mini
- Feet: hoverboard wheel mod (also by Steve Wagg)

## Build

[PlatformIO](https://platformio.org/) project, board `wemos_d1_mini32`:

```bash
pio run -t upload      # build + flash the firmware
pio run -t uploadfs    # upload the web UI (SPIFFS)
```

Or update over the air via the `/update` endpoint once connected to the WiFi Access Point (default SSID `ArtooR2D2`).

See [`spec.md`](spec.md) for the full functional specification and [`CLAUDE.md`](CLAUDE.md) for the project structure and conventions.

## Credits & attribution

The Artoo control board and the hoverboard wheel mod are the work of **Steve Wagg**. This repository is an independent firmware reimplementation and a community continuation. See [CREDITS.md](CREDITS.md) for the full story.

## License

[GPLv3](LICENSE).
