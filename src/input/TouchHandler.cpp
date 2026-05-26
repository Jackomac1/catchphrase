#include "TouchHandler.h"
#include <Wire.h>

static constexpr uint8_t  kTouchI2CAddr   = 0x3B;
static constexpr uint8_t  kReadTouchCmd[] = {0xB5, 0xAB, 0xA5, 0x5A,
                                              0x00, 0x00, 0x00, 0x08,
                                              0x00, 0x00, 0x00};

void TouchHandler::begin() {
    Wire.begin(BoardConfig::PIN_TOUCH_SDA, BoardConfig::PIN_TOUCH_SCL);
}

bool TouchHandler::readRaw(uint16_t& physX, uint16_t& physY) {
    Wire.beginTransmission(kTouchI2CAddr);
    Wire.write(kReadTouchCmd, sizeof(kReadTouchCmd));
    if (Wire.endTransmission(false) != 0) return false;

    if (Wire.requestFrom((uint8_t)kTouchI2CAddr, (uint8_t)8) != 8) return false;

    uint8_t data[8];
    for (int i = 0; i < 8; i++) data[i] = Wire.read();

    if (data[1] == 0) return false;  // no touch points

    physX = ((uint16_t)data[2] << 8) | data[3];
    physY = ((uint16_t)data[4] << 8) | data[5];
    return true;
}

TouchEvent TouchHandler::poll() {
    uint16_t px = 0, py = 0;
    bool touched = readRaw(px, py);

    TouchEvent ev;
    ev.touched = touched;
    if (!touched) {
        ev.x = ev.y = 0;
        return ev;
    }

    // Physical portrait (172×640) → logical landscape (640×172):
    // logical.x = PANEL_NATIVE_HEIGHT - 1 - physY
    // logical.y = physX
    ev.x = BoardConfig::PANEL_NATIVE_HEIGHT - 1 - (int)py;
    ev.y = (int)px;
    return ev;
}
