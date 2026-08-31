/**
 * Bird Feeder Camera - ESP32-CAM with OV5640
 *
 * Hardware:
 *   - APKLVSR ESP32-CAM + ESP32-CAM MB
 *   - OV5640 120-degree camera module
 *   - Xicoolee Solar Power Manager (5V-24V MPPT) with 18650 battery holder
 *   - POWO18B rechargeable 18650 batteries
 *   - Generic 5V 500mA solar panel
 *   - Camway IP67 enclosure
 *
 * Power strategy:
 *   Deep sleep between captures to maximise battery life on the solar setup.
 *   Wake on timer (default every 30 s). If battery voltage is low, extend
 *   the sleep interval to conserve charge.
 *
 * Board: AI Thinker ESP32-CAM  (select in Arduino IDE)
 * Partition scheme: Huge APP (3MB No OTA)
 */

#include "esp_camera.h"
#include "esp_sleep.h"
#include "esp_adc_cal.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "driver/adc.h"
#include "config.h"

// ── OV5640 pin map for AI-Thinker ESP32-CAM ───────────────────────────────
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26
#define CAM_PIN_SIOC    27
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// ── ADC pin wired to the Xicoolee VBAT sense divider ──────────────────────
// Connect the midpoint of a 100k/100k divider across the battery terminals
// to GPIO 33 (ADC1_CH5, safe to use in deep sleep wake stubs).
#define VBAT_ADC_PIN    33
#define VBAT_DIV_RATIO   2.0f   // 1:1 divider → multiply ADC result by 2

// ── Deep-sleep intervals ───────────────────────────────────────────────────
#define SLEEP_NORMAL_S   30     // seconds between captures (good battery)
#define SLEEP_LOW_BAT_S 120     // extended sleep when battery is low
#define VBAT_LOW_MV    3500     // threshold for "low battery" (mV)

// ── LED flash (built-in on GPIO 4) ────────────────────────────────────────
#define LED_FLASH_PIN    4

// ── RTC memory: persists through deep sleep ───────────────────────────────
RTC_DATA_ATTR static uint32_t bootCount = 0;

// ─────────────────────────────────────────────────────────────────────────
static camera_config_t camera_config = {
    .pin_pwdn      = CAM_PIN_PWDN,
    .pin_reset     = CAM_PIN_RESET,
    .pin_xclk      = CAM_PIN_XCLK,
    .pin_sscb_sda  = CAM_PIN_SIOD,
    .pin_sscb_scl  = CAM_PIN_SIOC,
    .pin_d7        = CAM_PIN_D7,
    .pin_d6        = CAM_PIN_D6,
    .pin_d5        = CAM_PIN_D5,
    .pin_d4        = CAM_PIN_D4,
    .pin_d3        = CAM_PIN_D3,
    .pin_d2        = CAM_PIN_D2,
    .pin_d1        = CAM_PIN_D1,
    .pin_d0        = CAM_PIN_D0,
    .pin_vsync     = CAM_PIN_VSYNC,
    .pin_href      = CAM_PIN_HREF,
    .pin_pclk      = CAM_PIN_PCLK,

    .xclk_freq_hz  = 20000000,
    .ledc_timer    = LEDC_TIMER_0,
    .ledc_channel  = LEDC_CHANNEL_0,

    .pixel_format  = PIXFORMAT_JPEG,
    // Use UXGA (1600×1200) for best detail; drop to SVGA if memory is tight
    .frame_size    = FRAMESIZE_UXGA,
    .jpeg_quality  = 12,        // 0–63, lower = better quality
    .fb_count      = 1,
    .grab_mode     = CAMERA_GRAB_WHEN_EMPTY,
};

// ─────────────────────────────────────────────────────────────────────────
uint32_t readBatteryMillivolts() {
    // Disable WiFi before ADC reading (ADC2 conflicts with WiFi; we use ADC1)
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_11);

    esp_adc_cal_characteristics_t chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11,
                             ADC_WIDTH_BIT_12, 1100, &chars);

    uint32_t raw = 0;
    for (int i = 0; i < 16; i++) {
        raw += adc1_get_raw(ADC1_CHANNEL_5);
    }
    raw /= 16;

    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(raw, &chars);
    return (uint32_t)(voltage_mv * VBAT_DIV_RATIO);
}

// ─────────────────────────────────────────────────────────────────────────
bool initCamera() {
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.printf("[CAM] Init failed: 0x%x\n", err);
        return false;
    }

    // OV5640-specific tuning
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 1);      // slight brightness boost outdoors
        s->set_saturation(s, 0);
        s->set_sharpness(s, 1);
        s->set_denoise(s, 1);
        s->set_awb_gain(s, 1);        // auto white balance
        s->set_exposure_ctrl(s, 1);   // auto exposure
        s->set_aec2(s, 1);            // AEC DSP
        s->set_gain_ctrl(s, 1);       // auto gain
        s->set_lenc(s, 1);            // lens correction (helps 120° lens)
        s->set_hmirror(s, 0);
        s->set_vflip(s, 0);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────
bool connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    const uint32_t timeout_ms = 15000;
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout_ms) {
        delay(250);
    }
    return WiFi.status() == WL_CONNECTED;
}

// ─────────────────────────────────────────────────────────────────────────
bool uploadImage(camera_fb_t *fb) {
    if (!fb || !fb->buf || fb->len == 0) return false;

    HTTPClient http;
    // NOTE: For HTTPS endpoints, the ESP32 HTTPClient requires a root CA
    // certificate or http.setInsecure() (which skips TLS verification).
    // To enable insecure HTTPS, uncomment the line below — but be aware
    // this makes the connection vulnerable to MITM attacks.
    // http.setInsecure();
    http.begin(UPLOAD_URL);
    http.addHeader("Content-Type", "image/jpeg");
    http.addHeader("X-Device-ID", DEVICE_ID);
    http.addHeader("X-Boot-Count", String(bootCount));

    int code = http.POST(fb->buf, fb->len);
    http.end();

    return (code == 200 || code == 201);
}

// ─────────────────────────────────────────────────────────────────────────
void goToSleep(uint32_t seconds) {
    Serial.printf("[PWR] Sleeping for %u s\n", seconds);
    Serial.flush();

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_camera_deinit();

    // Power down the camera sensor (PWDN is active-high on OV5640)
    gpio_set_level((gpio_num_t)CAM_PIN_PWDN, 1);
    gpio_hold_en((gpio_num_t)CAM_PIN_PWDN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup((uint64_t)seconds * 1000000ULL);
    esp_deep_sleep_start();
}

// ─────────────────────────────────────────────────────────────────────────
void setup() {
    bootCount++;
    Serial.begin(115200);
    delay(100);

    Serial.printf("\n[BOOT] #%u  wake cause: %d\n",
                  bootCount, esp_sleep_get_wakeup_cause());

    // ── Battery check ──────────────────────────────────────────────────
    uint32_t vbat_mv = readBatteryMillivolts();
    Serial.printf("[PWR] Battery: %u mV\n", vbat_mv);

    if (vbat_mv < VBAT_LOW_MV && vbat_mv > 500) {
        // vbat_mv > 500 guards against a floating ADC pin
        Serial.println("[PWR] Low battery — extending sleep");
        goToSleep(SLEEP_LOW_BAT_S);
        return; // never reached
    }

    // ── Camera ─────────────────────────────────────────────────────────
    if (!initCamera()) {
        goToSleep(SLEEP_NORMAL_S);
        return;
    }

    // Discard the first frame; OV5640 AEC needs a few frames to settle
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb) esp_camera_fb_return(fb);
    delay(300);

    fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[CAM] Capture failed");
        goToSleep(SLEEP_NORMAL_S);
        return;
    }
    Serial.printf("[CAM] Captured %zu bytes\n", fb->len);

    // ── WiFi & upload ──────────────────────────────────────────────────
    if (!connectWiFi()) {
        Serial.println("[NET] WiFi timeout — skipping upload");
        esp_camera_fb_return(fb);
        goToSleep(SLEEP_NORMAL_S);
        return;
    }

    bool ok = uploadImage(fb);
    esp_camera_fb_return(fb);
    Serial.printf("[NET] Upload %s\n", ok ? "OK" : "FAILED");

    goToSleep(SLEEP_NORMAL_S);
}

void loop() {
    // Not used — deep sleep returns to setup()
}
