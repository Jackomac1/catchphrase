#pragma once
#include "Colors.h"
#include <stdint.h>

// Logical display: 640 wide × 172 tall (landscape).
// Physical panel: 172 wide × 640 tall (portrait, rotated 90° in software).
static constexpr int kDisplayW = 640;
static constexpr int kDisplayH = 172;

// Layout zones
static constexpr int kHeaderH = 36;
static constexpr int kFooterH = 28;
static constexpr int kWordY   = kHeaderH;
static constexpr int kWordH   = kDisplayH - kHeaderH - kFooterH;

class GameDisplay {
public:
    void begin();

    // ---- Screen renderers ----
    void drawSetupScore(int selectedIndex, const int* options, int count);
    void drawCategorySelect(const char* categoryName, int index, int total, bool sdOk);
    void drawTeamColorSetup(int team, int selectedColor, uint16_t otherTeamColor);
    void drawCountdown(int number);
    void drawPlaying(const char* word, const char* category,
                     int scoreLeft, int scoreRight,
                     uint16_t colorLeft, uint16_t colorRight,
                     float urgency, bool paused);
    void drawExplosion();
    void drawScorePrompt(uint16_t colorLeft, uint16_t colorRight,
                         int scoreLeft, int scoreRight);
    void drawVictory(int winningTeam, uint16_t winColor, int score, int targetScore);
    void flashScreen(uint16_t color, int ms);

    // ---- Confetti ----
    void confettiInit();
    void confettiTick(uint32_t nowMs);

private:
    // Framebuffer in PSRAM: logical 640×172, RGB565
    uint16_t* fb_ = nullptr;

    // Confetti state
    struct Confetti { float x, y, dx, dy; uint16_t color; uint8_t size; };
    static constexpr int kConfettiCount = 120;
    Confetti confetti_[kConfettiCount];
    uint32_t lastConfettiMs_ = 0;

    // Low-level framebuffer ops
    void clear(uint16_t color = Colors::Black);
    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawChar(int x, int y, char c, uint16_t color, int scale);
    void drawString(const char* s, int x, int y, uint16_t color, int scale,
                    bool centerX = false, bool centerY = false);
    int  stringPixelWidth(const char* s, int scale) const;
    int  fitTextScale(const char* text, int maxW, int maxH) const;
    void flush();

    // UI helpers
    void drawWord(const char* text, int y, int h, uint16_t color);
    void drawScoreBox(int x, int w, int score, uint16_t color);
    void drawRedPulse(float urgency);
    void drawPauseButton(bool paused);
};
