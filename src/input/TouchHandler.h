#pragma once
#include <Arduino.h>
#include "board/BoardConfig.h"

struct TouchEvent {
    bool     touched;
    int      x;    // logical display x (0..639)
    int      y;    // logical display y (0..171)
};

class TouchHandler {
public:
    void       begin();
    TouchEvent poll();   // call every loop; returns current state

private:
    // I2C read helper
    bool readRaw(uint16_t& physX, uint16_t& physY);
};
