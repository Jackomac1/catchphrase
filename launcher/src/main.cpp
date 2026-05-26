// Boot launcher — factory partition (0x10000).
// Runs on every fresh power-on when otadata is cleared.
//
// UX: Hold BOOT button (GPIO 0) during 2.5s window → RSVP Nano (3 rapid blinks)
//     Release BOOT                                  → Catchphrase (1 long blink)
//
// No display driver needed — feedback via backlight GPIO blinks only.

#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

static constexpr int PIN_BOOT   = 0;  // active-low
static constexpr int PIN_LCD_BL = 8;  // backlight active-high

static void blink(int count, int onMs, int offMs) {
    for (int i = 0; i < count; i++) {
        digitalWrite(PIN_LCD_BL, HIGH);
        delay(onMs);
        digitalWrite(PIN_LCD_BL, LOW);
        if (i < count - 1) delay(offMs);
    }
}

static void bootInto(esp_partition_subtype_t subtype) {
    const esp_partition_t* target = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, subtype, nullptr);
    if (target) {
        esp_ota_set_boot_partition(target);
    }
    esp_restart();
}

void setup() {
    pinMode(PIN_BOOT, INPUT_PULLUP);
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, LOW);
    delay(50);  // debounce

    // Sample BOOT button for 2.5 seconds; heartbeat blink while waiting
    constexpr uint32_t kWindowMs = 2500;
    uint32_t start = millis();
    bool rsvpMode = false;
    uint32_t lastBlink = 0;

    while (millis() - start < kWindowMs) {
        rsvpMode = !digitalRead(PIN_BOOT);
        uint32_t now = millis();
        if (now - lastBlink > 250) {
            digitalWrite(PIN_LCD_BL, HIGH);
            delay(30);
            digitalWrite(PIN_LCD_BL, LOW);
            lastBlink = now;
        }
        delay(10);
    }

    // Signal choice: 3 rapid = RSVP Nano, 1 long = Catchphrase
    if (rsvpMode) {
        blink(3, 80, 80);
    } else {
        blink(1, 500, 0);
    }

    bootInto(rsvpMode
        ? ESP_PARTITION_SUBTYPE_APP_OTA_1
        : ESP_PARTITION_SUBTYPE_APP_OTA_0);
}

void loop() {}
