#pragma once
#include "LCD.h"
#include "Colors.h"

// Layout constants (logical 640×172)
static constexpr int kHeaderH = 36;
static constexpr int kFooterH = 28;
static constexpr int kWordY   = kHeaderH;
static constexpr int kWordH   = 172 - kHeaderH - kFooterH;  // 108px

class GameDisplay {
public:
    void begin(LGFX& lcd);

    // ---- Screen renderers ----
    void drawSetupScore(int selectedIndex, const int* options, int count);
    void drawCategorySelect(const char* categoryName, int index, int total,
                            bool sdOk);
    void drawTeamColorSetup(int team, int selectedColor, uint16_t otherTeamColor);
    void drawCountdown(int number);   // number = 3,2,1 or 0 for "GO!"
    void drawPlaying(const char* word, const char* category,
                     int scoreLeft, int scoreRight,
                     uint16_t colorLeft, uint16_t colorRight,
                     float urgency,   // 0..1, drives red pulse overlay
                     bool paused);
    void drawExplosion();
    void drawScorePrompt(uint16_t colorLeft, uint16_t colorRight,
                         int scoreLeft, int scoreRight);
    void drawVictory(int winningTeam, uint16_t winColor,
                     int score, int targetScore);
    void flashScreen(uint16_t color, int ms);

    // ---- Confetti (victory animation) ----
    void confettiTick(uint32_t nowMs);
    void confettiInit();

private:
    LGFX* lcd_ = nullptr;

    // Confetti state
    struct Confetti {
        float x, y, dx, dy;
        uint16_t color;
        uint8_t size;
    };
    static constexpr int kConfettiCount = 120;
    Confetti confetti_[kConfettiCount];
    uint32_t lastConfettiMs_ = 0;

    // Helpers
    void drawWord(const char* text, int y, int h, uint16_t color);
    void drawScoreBox(int x, int w, int score, uint16_t color, bool leftSide);
    void drawPauseButton(bool paused);
    void drawRedPulse(float urgency);
    int  fitTextScale(const char* text, int maxW, int maxH);
};
