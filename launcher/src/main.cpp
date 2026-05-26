//
// Boot launcher — lives on the factory partition (0x10000).
// Runs on every fresh power-on when otadata has been cleared.
//
// UX: Hold BOOT button (GPIO 0) during the 2-second window → RSVP Nano
//     Release it                                             → Catchphrase
//
// After selecting, writes otadata to point at ota_0 or ota_1, then reboots.
// That app then runs normally. On the NEXT hard power cycle (battery off/on
// or after the user runs the reset_to_launcher script), otadata is cleared
// again so this launcher runs again.
//

#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <LovyanGFX.hpp>

// --- Pin constants (matching BoardConfig) ---
static constexpr int PIN_BOOT  = 0;   // active-low
static constexpr int PIN_POWER = 16;  // active-low
static constexpr int PIN_LCD_CS    = 9;
static constexpr int PIN_LCD_SCLK  = 10;
static constexpr int PIN_LCD_DATA0 = 11;
static constexpr int PIN_LCD_DATA1 = 12;
static constexpr int PIN_LCD_DATA2 = 13;
static constexpr int PIN_LCD_DATA3 = 14;
static constexpr int PIN_LCD_RST   = 21;
static constexpr int PIN_LCD_BL    = 8;
static constexpr int PIN_TOUCH_SDA = 17;
static constexpr int PIN_TOUCH_SCL = 18;

// --- Minimal LGFX for the launcher ---
class LauncherLGFX : public lgfx::LGFX_Device {
public:
    LauncherLGFX() {
        {
            auto cfg = _bus.config();
            cfg.spi_host   = SPI3_HOST;
            cfg.freq_write = 40000000;  // conservative speed for launcher
            cfg.pin_sclk   = PIN_LCD_SCLK;
            cfg.pin_d0     = PIN_LCD_DATA0;
            cfg.pin_d1     = PIN_LCD_DATA1;
            cfg.pin_d2     = PIN_LCD_DATA2;
            cfg.pin_d3     = PIN_LCD_DATA3;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs          = PIN_LCD_CS;
            cfg.pin_rst         = PIN_LCD_RST;
            cfg.pin_busy        = -1;
            cfg.panel_width     = 172;
            cfg.panel_height    = 640;
            cfg.memory_width    = 172;
            cfg.memory_height   = 640;
            cfg.readable        = false;
            cfg.invert          = false;
            cfg.rgb_order       = false;
            cfg.dlen_16bit      = false;
            cfg.bus_shared      = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl      = PIN_LCD_BL;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 0;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        setPanel(&_panel);
    }
private:
    lgfx::Panel_AXS15231B _panel;
    lgfx::Bus_QSPI        _bus;
    lgfx::Light_PWM       _light;
};

static LauncherLGFX lcd;

static void drawBootMenu(bool rsvpSelected, uint32_t remainingMs) {
    uint32_t elapsed   = 2500 - remainingMs;
    uint32_t barWidth  = (uint32_t)(640.f * elapsed / 2500.f);

    lcd.fillScreen(0x0000);  // black

    // Title
    lcd.setTextDatum(MC_DATUM);
    lcd.setTextColor(0xFFFF, 0x0000);
    lcd.setTextSize(2);
    lcd.drawString("BOOT SELECTOR", 320, 30);

    // Two options
    uint16_t catchColor = rsvpSelected ? 0x4208 : 0x07E0;  // dim or green
    uint16_t rsvpColor  = rsvpSelected ? 0xFD20 : 0x4208;  // orange or dim

    lcd.setTextSize(3);
    lcd.setTextColor(catchColor, 0x0000);
    lcd.drawString("Catchphrase", 160, 90);

    lcd.setTextColor(rsvpColor, 0x0000);
    lcd.drawString("RSVP Nano", 480, 90);

    // Instruction
    lcd.setTextSize(1);
    lcd.setTextColor(0x7BEF, 0x0000);
    lcd.drawString(rsvpSelected ? "Hold BOOT to launch RSVP Nano" : "Release BOOT for Catchphrase",
                   320, 130);

    // Progress bar
    lcd.fillRect(0, 155, 640, 8, 0x2104);
    lcd.fillRect(0, 155, (int)barWidth, 8, rsvpSelected ? 0xFD20 : 0x07E0);
}

static void bootIntoPartition(esp_partition_subtype_t subtype) {
    const esp_partition_t* target = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, subtype, nullptr);
    if (!target) {
        // Partition not found — fall through to factory next boot
        return;
    }
    esp_ota_set_boot_partition(target);
    esp_restart();
}

void setup() {
    Serial.begin(115200);

    // Read BOOT button immediately before any init
    pinMode(PIN_BOOT, INPUT_PULLUP);
    delay(50);  // settle

    // Init display
    lcd.init();
    lcd.setRotation(1);
    lcd.setBrightness(180);

    // Selection loop: 2.5 seconds
    constexpr uint32_t kWindowMs = 2500;
    uint32_t startMs = millis();
    bool rsvpMode = false;

    while (millis() - startMs < kWindowMs) {
        bool bootHeld = !digitalRead(PIN_BOOT);
        rsvpMode = bootHeld;
        uint32_t remaining = kWindowMs - (millis() - startMs);
        drawBootMenu(rsvpMode, remaining);
        delay(50);
    }

    // Launch selected app
    lcd.fillScreen(0x0000);
    lcd.setTextDatum(MC_DATUM);
    lcd.setTextColor(0xFFFF, 0x0000);
    lcd.setTextSize(2);
    lcd.drawString(rsvpMode ? "Launching RSVP Nano..." : "Launching Catchphrase...", 320, 86);
    delay(300);

    bootIntoPartition(rsvpMode
        ? ESP_PARTITION_SUBTYPE_APP_OTA_1
        : ESP_PARTITION_SUBTYPE_APP_OTA_0);

    // If neither partition exists, just loop (shouldn't happen after full flash)
    lcd.fillScreen(0x0000);
    lcd.setTextColor(0xF800, 0x0000);
    lcd.drawString("No app partition found!", 320, 86);
    lcd.drawString("Flash Catchphrase and RSVP Nano first.", 320, 110);
}

void loop() {}
