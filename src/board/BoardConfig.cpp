#include "BoardConfig.h"

namespace BoardConfig {

static bool tca9554Ok = false;
static uint8_t tcaOutput = 0x00;

static bool tcaWrite(uint8_t reg, uint8_t val) {
    Wire1.beginTransmission(TCA9554_ADDRESS);
    Wire1.write(reg);
    Wire1.write(val);
    return Wire1.endTransmission() == 0;
}

static void tcaSetBit(uint8_t bit, bool on) {
    if (on) tcaOutput |= (1 << bit);
    else    tcaOutput &= ~(1 << bit);
    tcaWrite(0x01, tcaOutput);  // output port register
}

void begin() {
    // System I2C bus
    Wire1.begin(PIN_SYS_SDA, PIN_SYS_SCL, 400000);

    // Touch I2C bus
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, 400000);

    // Configure TCA9554: all pins as output
    tca9554Ok = tcaWrite(0x03, 0x00);  // config register: all outputs
    if (tca9554Ok) {
        tcaWrite(0x01, tcaOutput);     // set all outputs low initially
    }

    // Hold power so device stays on when USB removed
    holdPower();
}

void holdPower() {
    tcaSetBit(TCA_BIT_PWR_HOLD, true);
}

void enableAudioAmp() {
    tcaSetBit(TCA_BIT_AMP_EN, true);
}

void disableAudioAmp() {
    tcaSetBit(TCA_BIT_AMP_EN, false);
}

void setBrightness(uint8_t percent) {
    // Backlight is driven by LovyanGFX via PWM on PIN_LCD_BL
    // This is a no-op here; call lcd.setBrightness() directly.
    (void)percent;
}

}  // namespace BoardConfig
