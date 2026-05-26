#pragma once
#include <Arduino.h>
#include <Wire.h>

namespace BoardConfig {

// --- Buttons ---
static constexpr int PIN_BOOT   = 0;   // active-low
static constexpr int PIN_POWER  = 16;  // active-low

// --- LCD (QSPI) ---
static constexpr int PIN_LCD_CS         = 9;
static constexpr int PIN_LCD_SCLK       = 10;
static constexpr int PIN_LCD_DATA0      = 11;
static constexpr int PIN_LCD_DATA1      = 12;
static constexpr int PIN_LCD_DATA2      = 13;
static constexpr int PIN_LCD_DATA3      = 14;
static constexpr int PIN_LCD_RST        = 21;
static constexpr int PIN_LCD_BL         = 8;
static constexpr int PIN_LCD_BACKLIGHT  = 8;  // alias used by axs15231b driver

// --- SD card (1-bit MMC) ---
static constexpr int PIN_SD_CLK = 41;
static constexpr int PIN_SD_CMD = 39;
static constexpr int PIN_SD_D0  = 40;

// --- I2C bus 0: touch (Wire) ---
static constexpr int PIN_TOUCH_SDA = 17;
static constexpr int PIN_TOUCH_SCL = 18;

// --- I2C bus 1: system (Wire1) ---
static constexpr int PIN_SYS_SDA = 47;
static constexpr int PIN_SYS_SCL = 48;

// --- I2S audio ---
static constexpr int PIN_I2S_MCLK = 7;
static constexpr int PIN_I2S_BCLK = 15;
static constexpr int PIN_I2S_WS   = 46;
static constexpr int PIN_I2S_DIN  = 6;
static constexpr int PIN_I2S_DOUT = 45;

// --- Battery ADC ---
static constexpr int PIN_BATT_ADC = 4;

// --- I2C peripheral addresses ---
static constexpr uint8_t TCA9554_ADDRESS = 0x20;
static constexpr uint8_t ES8311_ADDRESS  = 0x18;

// --- Display dimensions ---
static constexpr int DISPLAY_WIDTH       = 640;  // logical landscape
static constexpr int DISPLAY_HEIGHT      = 172;
static constexpr int PANEL_NATIVE_WIDTH  = 172;  // physical portrait
static constexpr int PANEL_NATIVE_HEIGHT = 640;

// --- Display orientation ---
static constexpr bool UI_ROTATED_180 = false;

enum class UiOrientation {
    Portrait,
    PortraitFlipped,
    Landscape,
    LandscapeFlipped,
};

// --- TCA9554 bit positions ---
static constexpr uint8_t TCA_BIT_BATT_SENSE = 0;
static constexpr uint8_t TCA_BIT_PWR_HOLD   = 1;
static constexpr uint8_t TCA_BIT_AMP_EN     = 2;

void begin();
void enableAudioAmp();
void disableAudioAmp();
void holdPower();
void setBrightness(uint8_t percent);

}  // namespace BoardConfig
