#pragma once
#include <Arduino.h>

class ButtonHandler {
public:
    explicit ButtonHandler(int pin);
    void begin();
    void update(uint32_t nowMs);

    bool isHeld() const         { return held_; }
    bool wasPressedEvent();     // returns true once per press
    bool wasReleasedEvent();    // returns true once per release
    uint32_t heldDurationMs(uint32_t nowMs) const;

private:
    int      pin_;
    bool     held_           = false;
    bool     pressedEvent_   = false;
    bool     releasedEvent_  = false;
    uint32_t pressStartMs_   = 0;
    uint32_t lastEdgeMs_     = 0;
    uint32_t lastHoldDurMs_  = 0;
};
