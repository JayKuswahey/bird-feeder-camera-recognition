Purpose
This file orients a Copilot cloud agent that sees this repository for the first time. It gives compact, high-value facts about layout, build/validation steps, and constraints so the agent can make safe, small changes without unnecessary searching.

High-level summary
- What this repo does: Solar-powered bird-feeder camera project. Firmware for an ESP32-CAM posts JPEGs to a server; repository also contains wiring docs.
- Size & type: Small, single-purpose repo (~10 files, mostly docs + one Arduino sketch). Languages: Markdown, Arduino C/C++ (".ino" + ".h"). No tests or compiled artifacts in repo.

Project layout (high-priority)
- Root files: README.md, LICENSE, .gitignore, docs/, firmware/
- Firmware: firmware/bird_feeder_cam/
  - bird_feeder_cam.ino — main Arduino sketch (ESP32-CAM, deep-sleep, ADC battery read, HTTP POST upload). Important: uses `config.h` for secrets and upload URL.
  - config.h — user configuration: `WIFI_SSID`, `WIFI_PASSWORD`, `UPLOAD_URL`, `DEVICE_ID`.
- Docs: docs/hardware_wiring.md and docs/Frigate-install-docker.md — hardware and integration notes.

Key facts agents need to act safely
- Do not change firmware/bird_feeder_cam/config.h in a PR except to add defaults or make non-secret refactors. This file contains secrets (WiFi, server URL) and should not be populated with real credentials.
- Firmware build target: AI Thinker ESP32-CAM board. Partition scheme: “Huge APP (3MB No OTA)”. This must be used when compiling in the Arduino IDE or PlatformIO.
- HTTPS: The firmware supports HTTPS but requires a root CA or calling `http.setInsecure()` in `bird_feeder_cam.ino` (the code comments explain this). Changing cert behavior is security-sensitive — avoid enabling insecure TLS in PRs unless asked.

Build / validate (what to run locally before opening a PR)
- Quick sanity: read-only doc changes need no build; firmware changes require compilation and smoke testing on device.
- Recommended local validation steps:
  1) Edit `firmware/bird_feeder_cam/config.h` with dummy but syntactically valid values (do NOT commit real secrets).
  2) Open `bird_feeder_cam.ino` in Arduino IDE with ESP32 core installed (https://github.com/espressif/arduino-esp32) and select Board → AI Thinker ESP32-CAM and Partition Scheme → Huge APP (3MB No OTA). Compile.
  3) Alternatively use PlatformIO / `platformio run` after creating an appropriate `platformio.ini` (not present by default).
  4) Watch for these common build issues: missing ESP32 Arduino core, wrong board selection, insufficient partition size (causes build errors when using large libs or framesize UXGA).

CI / workflows
- There are no GitHub Actions or CI workflows in this repo. The maintainer expects local compile verification for firmware changes. Assume no automated tests will block a PR.

Safe-change guidance (short)
- For docs/README edits: update README.md and related docs; open PR.
- For firmware changes:
  - Always compile locally against AI Thinker ESP32-CAM before PR.
  - Do not include secrets in commits; prefer updating `config.h` examples or adding `config.example.h` if introducing schema changes.
  - When changing network/TLS behavior, document the security tradeoffs in the PR body and update README.

Repository inventory (top-priority file list)
- README.md — project overview, firmware quick-start (edit `config.h`, flash to AI Thinker ESP32-CAM, partition scheme). See [README.md](README.md) for details.
- LICENSE — license text.
- docs/ — hardware wiring and Frigate notes: [docs/hardware_wiring.md](docs/hardware_wiring.md), [docs/Frigate-install-docker.md](docs/Frigate-install-docker.md).
- firmware/bird_feeder_cam/bird_feeder_cam.ino — main firmware. Key excerpts:
  - Board & partition comment: "Board: AI Thinker ESP32-CAM  (select in Arduino IDE) \nPartition scheme: Huge APP (3 MB No OTA)"
  - HTTP upload: `http.begin(UPLOAD_URL)`; comment about `http.setInsecure()` for HTTPS.
- firmware/bird_feeder_cam/config.h — config placeholders; contains:
  - `#define WIFI_SSID     "YOUR_WIFI_SSID"`
  - `#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"`
  - `#define UPLOAD_URL    "http://YOUR_SERVER/upload"`
  - `#define DEVICE_ID     "feeder-01"`

Rules of trust for the agent (follow these exactly)
1) Trust this file as the single source of repo-level facts. Only re-run repo-wide searches when you cannot complete a change using this guidance.
2) Do not add secrets to commits. If a change needs new config values, add them to a committed example file (e.g., `config.example.h`) and document how the maintainer should populate secrets locally.
3) For firmware code edits, compile locally using the Arduino IDE before proposing a PR. Include the exact compiler output in the PR if you changed build-critical code.

Troubleshooting notes
- Common failures: missing ESP32 Arduino core, wrong board/partition selection, ADC2 vs ADC1 pin conflicts (this project uses ADC1_CH5 / GPIO33), and running out of RAM with UXGA frame size. If you see memory errors, try lowering `.frame_size` to `FRAMESIZE_SVGA` in `bird_feeder_cam.ino`.

If anything in this file is inconsistent with repository contents, search only the paths listed above. Prefer local compile and manual device testing over speculative automated runs.

End.
