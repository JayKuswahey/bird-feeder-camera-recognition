/**
 * config.h — User configuration for bird feeder camera
 *
 * Copy this file or edit it directly with your WiFi credentials and
 * upload endpoint before flashing.
 */

#pragma once

// ── WiFi ──────────────────────────────────────────────────────────────────
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ── Upload endpoint ───────────────────────────────────────────────────────
// The firmware HTTP-POSTs the JPEG to this URL.
// HTTP example: "http://192.168.1.100:8080/upload"
// HTTPS note:   ESP32 HTTPClient requires a root CA certificate for verified
//               TLS. To use HTTPS without a certificate, uncomment the
//               http.setInsecure() line in bird_feeder_cam.ino (insecure —
//               no server authentication).
#define UPLOAD_URL    "http://YOUR_SERVER/upload"

// ── Device identity ───────────────────────────────────────────────────────
// Sent as X-Device-ID header so the server can identify multiple feeders.
#define DEVICE_ID     "feeder-01"
