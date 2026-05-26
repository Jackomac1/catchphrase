#pragma once
#include <stdint.h>

static inline constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

namespace Colors {
    static constexpr uint16_t Black    = 0x0000;
    static constexpr uint16_t White    = 0xFFFF;
    static constexpr uint16_t Red      = 0xF800;
    static constexpr uint16_t Green    = rgb565(0, 200, 60);
    static constexpr uint16_t Blue     = rgb565(30, 120, 255);
    static constexpr uint16_t Yellow   = rgb565(255, 220, 0);
    static constexpr uint16_t Orange   = rgb565(255, 140, 0);
    static constexpr uint16_t Purple   = rgb565(160, 50, 220);
    static constexpr uint16_t DimGray  = rgb565(40, 40, 40);
    static constexpr uint16_t Gray     = rgb565(100, 100, 100);
    static constexpr uint16_t LightGray= rgb565(180, 180, 180);
}

static constexpr uint16_t kTeamColorOptions[] = {
    Colors::Red, Colors::Blue, Colors::Green,
    Colors::Yellow, Colors::Orange, Colors::Purple
};
static constexpr int kTeamColorCount = 6;
