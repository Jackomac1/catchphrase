#include "ButtonHandler.h"

ButtonHandler::ButtonHandler(int pin) : pin_(pin) {}

void ButtonHandler::begin() {
    pinMode(pin_, INPUT_PULLUP);
}

void ButtonHandler::update(uint32_t nowMs) {
    const bool currentHeld = !digitalRead(pin_);  // active-low
    if (currentHeld != held_) {
        held_       = currentHeld;
        lastEdgeMs_ = nowMs;
        if (held_) {
            pressedEvent_ = true;
            pressStartMs_ = nowMs;
        } else {
            releasedEvent_ = true;
            lastHoldDurMs_ = nowMs - pressStartMs_;
        }
    }
}

bool ButtonHandler::wasPressedEvent() {
    if (pressedEvent_) { pressedEvent_ = false; return true; }
    return false;
}

bool ButtonHandler::wasReleasedEvent() {
    if (releasedEvent_) { releasedEvent_ = false; return true; }
    return false;
}

uint32_t ButtonHandler::heldDurationMs(uint32_t nowMs) const {
    return held_ ? (nowMs - pressStartMs_) : 0;
}
