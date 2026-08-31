# Bird Feeder Camera Recognition

Solar-powered, 3D-printed bird feeder with an ESP32-CAM, OV5640 120° camera,
bird-species recognition, and push notifications.

## Hardware

| Component | Part |
|-----------|------|
| Solar panel | Generic 5 V / 500 mA (+/− wires) |
| Power manager | Xicoolee Solar Energy/Power Manager 5 V–24 V MPPT, JST output |
| Battery | POWO18B 18650 rechargeable cells |
| Microcontroller | APKLVSR ESP32-CAM + ESP32-CAM MB |
| Camera | OV5640 120° FOV (bundled with ESP32-CAM) |
| Enclosure | Camway IP67 storage box |

See [docs/hardware_wiring.md](docs/hardware_wiring.md) for full wiring details
and power budget.

## Firmware

The Arduino sketch lives in
[`firmware/bird_feeder_cam/`](firmware/bird_feeder_cam/).

Key features:
- **Deep sleep** between captures (default 30 s) — maximises battery life
- **Low-battery extension** — sleep stretches to 120 s below 3.5 V
- **OV5640 tuning** — auto-exposure, auto-gain, lens-correction for wide FOV
- **HTTP upload** — POSTs JPEG to a configurable endpoint for server-side
  recognition

Quick start:
1. Edit `firmware/bird_feeder_cam/config.h` with your WiFi and server URL.
2. Flash to **AI Thinker ESP32-CAM** with partition scheme **Huge APP (3 MB No OTA)**.

## Licence

See [LICENSE](LICENSE).
