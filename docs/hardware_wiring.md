# Hardware & Wiring Guide

## Component list

| Component | Details |
|-----------|---------|
| Solar panel | Generic 5 V / 500 mA (bare +/− wires) |
| Power manager | Xicoolee Solar Energy/Power Manager, 5 V–24 V input, MPPT, 16540 battery holder, JST output |
| Battery | POWO18B 18650 rechargeable cells (fit the 16540 holder with a 16540→18650 sleeve if needed) |
| Microcontroller | APKLVSR ESP32-CAM + ESP32-CAM MB breakout board |
| Camera | OV5640, 120° FOV, mounted on the ESP32-CAM |
| Enclosure | Camway IP67 storage box |

---

## Wiring overview

```
Solar panel (+) ──► Xicoolee IN+
Solar panel (−) ──► Xicoolee IN−

Xicoolee OUT JST (+, 5 V) ──► ESP32-CAM MB 5 V pin
Xicoolee OUT JST (−, GND) ──► ESP32-CAM MB GND pin

Battery voltage sense (optional, for low-battery sleep):
  100 kΩ from VBAT+ ──┬──► GPIO 33 (ADC1_CH5)
  100 kΩ to GND   ──┘
```

> **Note:** The ESP32-CAM MB board has a micro-USB socket and 5 V/GND header
> pins that accept the JST cable from the Xicoolee module directly (with the
> correct JST-PH 2-pin pigtail).

---

## Power budget

| State | Current draw | Notes |
|-------|-------------|-------|
| Deep sleep | ~5–10 mA | Camera powered down, only RTC active |
| Wake + capture | ~180–250 mA peak | Camera + WiFi active (~3–5 s) |
| Idle WiFi | ~80–120 mA | WiFi up, camera off |

**5 V / 500 mA panel** produces up to 2.5 W.  With 30-second capture intervals
the duty cycle is roughly 5 s active / 25 s sleep, so average current is well
under 50 mA — comfortable for the panel output even on partly cloudy days.

---

## Battery notes

- The Xicoolee holder is labelled "16540"; 18650 cells (POWO18B) are slightly
  longer (65 mm vs 54 mm).  Use a 16540→18650 adapter sleeve or verify your
  specific Xicoolee model accepts 18650 before inserting.
- The MPPT controller will cut off charging when the cell is full and resume
  when solar energy is available; no additional protection circuitry is needed.
- The firmware reads the raw battery voltage via a resistor divider on GPIO 33
  and extends the deep-sleep interval to 120 s when voltage drops below 3.5 V.

---

## Enclosure

Mount the solar panel outside the IP67 Camway box with a small cable gland
(M16 recommended) for the +/− wires entering the box.  Route the wires to the
Xicoolee board inside the sealed box.  The ESP32-CAM sits in the same box with
the OV5640 lens pointing through a small hole sealed with a clear silicone dome
or short acrylic tube sealed with silicone.

---

## Firmware

See [`firmware/bird_feeder_cam/`](../firmware/bird_feeder_cam/):

1. Edit **`config.h`** with your WiFi SSID, password, and upload URL.
2. Open `bird_feeder_cam.ino` in Arduino IDE (≥ 2.x).
3. Board: **AI Thinker ESP32-CAM** | Partition: **Huge APP (3 MB No OTA)**.
4. Flash via the ESP32-CAM MB USB port (hold IO0 low on first upload).
5. After flashing, remove the IO0 jumper and press reset.
