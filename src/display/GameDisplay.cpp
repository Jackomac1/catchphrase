#include "GameDisplay.h"
#include <Arduino.h>
#include <math.h>

static constexpr int W = 640;
static constexpr int H = 172;

void GameDisplay::begin(LGFX& lcd) {
    lcd_ = &lcd;
    lcd_->init();
    // Rotation 1 = 90° CW → 640×172 landscape. Use 3 if image is mirrored.
    lcd_->setRotation(1);
    lcd_->setBrightness(200);
    lcd_->fillScreen(Colors::Black);
}

// ---------------------------------------------------------------------------
// Setup: choose target score
// ---------------------------------------------------------------------------
void GameDisplay::drawSetupScore(int selectedIndex, const int* options, int count) {
    lcd_->fillScreen(Colors::Black);
    lcd_->setTextColor(Colors::White, Colors::Black);
    lcd_->setTextSize(2);
    lcd_->setTextDatum(MC_DATUM);
    lcd_->drawString("First to how many?", W / 2, 30);

    int btnW = 80, btnH = 42, gap = 18;
    int totalW = count * btnW + (count - 1) * gap;
    int startX = (W - totalW) / 2;

    for (int i = 0; i < count; i++) {
        int x = startX + i * (btnW + gap);
        int y = 70;
        uint16_t bg  = (i == selectedIndex) ? Colors::Orange : Colors::DimGray;
        uint16_t txt = Colors::White;
        lcd_->fillRoundRect(x, y, btnW, btnH, 8, bg);
        lcd_->setTextSize(3);
        lcd_->drawNumber(options[i], x + btnW / 2, y + btnH / 2);
    }

    lcd_->setTextSize(1);
    lcd_->setTextColor(Colors::Gray, Colors::Black);
    lcd_->drawString("Tap a number, then tap again to confirm", W / 2, 140);
}

// ---------------------------------------------------------------------------
// Category select carousel
// ---------------------------------------------------------------------------
void GameDisplay::drawCategorySelect(const char* categoryName, int index, int total,
                                     bool sdOk) {
    lcd_->fillScreen(Colors::Black);
    lcd_->setTextDatum(MC_DATUM);

    if (!sdOk) {
        lcd_->setTextColor(Colors::Red, Colors::Black);
        lcd_->setTextSize(2);
        lcd_->drawString("No SD card found!", W / 2, H / 2 - 10);
        lcd_->setTextSize(1);
        lcd_->setTextColor(Colors::Gray, Colors::Black);
        lcd_->drawString("Insert SD card with /categories/*.txt files", W / 2, H / 2 + 20);
        return;
    }

    lcd_->setTextColor(Colors::Gray, Colors::Black);
    lcd_->setTextSize(1);
    lcd_->drawString("Choose Category", W / 2, 14);

    // Arrows
    lcd_->setTextColor(Colors::White, Colors::Black);
    lcd_->setTextSize(3);
    lcd_->drawString("<", 30, H / 2);
    lcd_->drawString(">", W - 30, H / 2);

    // Category name
    lcd_->setTextSize(4);
    lcd_->setTextColor(Colors::Yellow, Colors::Black);
    lcd_->drawString(categoryName, W / 2, H / 2);

    // Page indicator
    lcd_->setTextSize(1);
    lcd_->setTextColor(Colors::Gray, Colors::Black);
    char buf[24];
    snprintf(buf, sizeof(buf), "%d / %d", index + 1, total);
    lcd_->drawString(buf, W / 2, H - 14);

    lcd_->drawString("Tap center to start  |  Hold POWER 3s = RSVP Nano", W / 2, H - 26);
}

// ---------------------------------------------------------------------------
// Team color setup
// ---------------------------------------------------------------------------
void GameDisplay::drawTeamColorSetup(int team, int selectedColor, uint16_t otherTeamColor) {
    lcd_->fillScreen(Colors::Black);
    lcd_->setTextDatum(MC_DATUM);

    char title[32];
    snprintf(title, sizeof(title), "Team %d color", team + 1);
    lcd_->setTextColor(Colors::White, Colors::Black);
    lcd_->setTextSize(2);
    lcd_->drawString(title, W / 2, 18);

    int swatchSize = 44, gap = 20;
    int totalW = kTeamColorCount * swatchSize + (kTeamColorCount - 1) * gap;
    int startX = (W - totalW) / 2;

    for (int i = 0; i < kTeamColorCount; i++) {
        int x = startX + i * (swatchSize + gap);
        int y = 60;
        uint16_t c = kTeamColorOptions[i];
        lcd_->fillRoundRect(x, y, swatchSize, swatchSize, 6, c);
        if (i == selectedColor)
            lcd_->drawRoundRect(x - 2, y - 2, swatchSize + 4, swatchSize + 4, 8, Colors::White);
        if (c == otherTeamColor)
            lcd_->drawRoundRect(x, y, swatchSize, swatchSize, 6, Colors::Gray);
    }

    lcd_->setTextSize(1);
    lcd_->setTextColor(Colors::Gray, Colors::Black);
    lcd_->drawString("Tap a color. Tap selected to confirm.", W / 2, 140);
}

// ---------------------------------------------------------------------------
// Countdown 3, 2, 1, GO
// ---------------------------------------------------------------------------
void GameDisplay::drawCountdown(int number) {
    lcd_->fillScreen(Colors::Black);
    lcd_->setTextDatum(MC_DATUM);
    if (number > 0) {
        lcd_->setTextColor(Colors::White, Colors::Black);
        lcd_->setTextSize(8);
        lcd_->drawNumber(number, W / 2, H / 2);
    } else {
        lcd_->setTextColor(Colors::Green, Colors::Black);
        lcd_->setTextSize(6);
        lcd_->drawString("GO!", W / 2, H / 2);
    }
}

// ---------------------------------------------------------------------------
// Main gameplay screen
// ---------------------------------------------------------------------------
void GameDisplay::drawPlaying(const char* word, const char* category,
                               int scoreLeft, int scoreRight,
                               uint16_t colorLeft, uint16_t colorRight,
                               float urgency, bool paused) {
    lcd_->fillScreen(Colors::Black);

    // Header bar background
    lcd_->fillRect(0, 0, W, kHeaderH, Colors::DimGray);

    // Score boxes
    drawScoreBox(0, 110, scoreLeft, colorLeft, true);
    drawScoreBox(W - 110, 110, scoreRight, colorRight, false);

    // Category label (center of header)
    lcd_->setTextDatum(MC_DATUM);
    lcd_->setTextColor(Colors::LightGray, Colors::DimGray);
    lcd_->setTextSize(1);
    lcd_->drawString(category, W / 2, kHeaderH / 2);

    // Large word in center area
    uint16_t wordColor = Colors::White;
    drawWord(word, kWordY, kWordH, wordColor);

    // Red urgency pulse overlay
    if (urgency > 0.4f) {
        drawRedPulse(urgency);
    }

    // Pause button
    drawPauseButton(paused);

    // Paused overlay
    if (paused) {
        lcd_->fillRect(0, 0, W, H, 0x0000);  // black out
        lcd_->setTextDatum(MC_DATUM);
        lcd_->setTextColor(Colors::White, Colors::Black);
        lcd_->setTextSize(4);
        lcd_->drawString("PAUSED", W / 2, H / 2 - 16);
        lcd_->setTextSize(1);
        lcd_->setTextColor(Colors::Gray, Colors::Black);
        lcd_->drawString("Tap to resume", W / 2, H / 2 + 20);
    }
}

// ---------------------------------------------------------------------------
// Explosion screen
// ---------------------------------------------------------------------------
void GameDisplay::drawExplosion() {
    lcd_->fillScreen(Colors::Red);
    lcd_->setTextDatum(MC_DATUM);
    lcd_->setTextColor(Colors::White, Colors::Red);
    lcd_->setTextSize(6);
    lcd_->drawString("BOOM!", W / 2, H / 2);
}

// ---------------------------------------------------------------------------
// Score prompt (tap left or right to award point)
// ---------------------------------------------------------------------------
void GameDisplay::drawScorePrompt(uint16_t colorLeft, uint16_t colorRight,
                                  int scoreLeft, int scoreRight) {
    lcd_->fillScreen(Colors::Black);

    // Left half
    lcd_->fillRect(0, 0, W / 2 - 2, H, colorLeft);
    // Divider
    lcd_->fillRect(W / 2 - 2, 0, 4, H, Colors::White);
    // Right half
    lcd_->fillRect(W / 2 + 2, 0, W / 2 - 2, H, colorRight);

    lcd_->setTextDatum(MC_DATUM);
    lcd_->setTextSize(4);
    lcd_->setTextColor(Colors::White);

    // Left team score display
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", scoreLeft);
    lcd_->drawString(buf, W / 4, H / 2 - 20);

    // Right team score
    snprintf(buf, sizeof(buf), "%d", scoreRight);
    lcd_->drawString(buf, 3 * W / 4, H / 2 - 20);

    lcd_->setTextSize(1);
    lcd_->drawString("Your team scored?", W / 2, H - 16);
    lcd_->setTextSize(2);
    lcd_->drawString("Tap your side!", W / 2, H - 30);
}

// ---------------------------------------------------------------------------
// Victory screen with confetti animation
// ---------------------------------------------------------------------------
void GameDisplay::drawVictory(int winningTeam, uint16_t winColor,
                               int score, int targetScore) {
    lcd_->fillScreen(Colors::Black);

    // Confetti
    for (int i = 0; i < kConfettiCount; i++) {
        Confetti& c = confetti_[i];
        lcd_->fillRect((int)c.x, (int)c.y, c.size, c.size, c.color);
    }

    lcd_->setTextDatum(MC_DATUM);
    lcd_->setTextColor(winColor, Colors::Black);
    lcd_->setTextSize(3);
    char buf[32];
    snprintf(buf, sizeof(buf), "Team %d Wins!", winningTeam + 1);
    lcd_->drawString(buf, W / 2, 40);

    lcd_->setTextColor(Colors::White, Colors::Black);
    lcd_->setTextSize(2);
    snprintf(buf, sizeof(buf), "%d - %d", score, targetScore);
    lcd_->drawString(buf, W / 2, 80);

    lcd_->setTextColor(Colors::Gray, Colors::Black);
    lcd_->setTextSize(1);
    lcd_->drawString("Tap anywhere to play again", W / 2, H - 16);
}

void GameDisplay::confettiInit() {
    for (int i = 0; i < kConfettiCount; i++) {
        confetti_[i].x     = random(W);
        confetti_[i].y     = random(H);
        confetti_[i].dx    = (random(40) - 20) / 10.0f;
        confetti_[i].dy    = (random(20) + 5)  / 10.0f;
        confetti_[i].size  = random(3, 8);
        confetti_[i].color = kTeamColorOptions[random(kTeamColorCount)];
    }
}

void GameDisplay::confettiTick(uint32_t nowMs) {
    if (nowMs - lastConfettiMs_ < 50) return;
    lastConfettiMs_ = nowMs;
    for (int i = 0; i < kConfettiCount; i++) {
        confetti_[i].x += confetti_[i].dx;
        confetti_[i].y += confetti_[i].dy;
        if (confetti_[i].y > H) {
            confetti_[i].y = -confetti_[i].size;
            confetti_[i].x = random(W);
        }
        if (confetti_[i].x < 0) confetti_[i].x = W;
        if (confetti_[i].x > W) confetti_[i].x = 0;
    }
}

// ---------------------------------------------------------------------------
// Flash screen
// ---------------------------------------------------------------------------
void GameDisplay::flashScreen(uint16_t color, int ms) {
    lcd_->fillScreen(color);
    delay(ms);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void GameDisplay::drawWord(const char* text, int y, int h, uint16_t color) {
    lcd_->setTextDatum(MC_DATUM);
    lcd_->setTextColor(color, Colors::Black);

    int scale = fitTextScale(text, W - 20, h - 10);
    lcd_->setTextSize(scale);
    lcd_->drawString(text, W / 2, y + h / 2);
}

int GameDisplay::fitTextScale(const char* text, int maxW, int maxH) {
    int len = strlen(text);
    // LovyanGFX default char width ≈ 6px per char at size 1
    for (int scale = 7; scale >= 1; scale--) {
        if (len * 6 * scale <= maxW && 8 * scale <= maxH)
            return scale;
    }
    return 1;
}

void GameDisplay::drawScoreBox(int x, int w, int score, uint16_t color, bool leftSide) {
    lcd_->fillRect(x, 0, w, kHeaderH, color);
    lcd_->setTextColor(Colors::White, color);
    lcd_->setTextSize(3);
    lcd_->setTextDatum(MC_DATUM);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", score);
    lcd_->drawString(buf, x + w / 2, kHeaderH / 2);
}

void GameDisplay::drawPauseButton(bool paused) {
    int btnW = 70, btnH = 20;
    int bx = (W - btnW) / 2;
    int by = H - kFooterH + 4;
    lcd_->fillRoundRect(bx, by, btnW, btnH, 5, Colors::DimGray);
    lcd_->setTextColor(Colors::LightGray, Colors::DimGray);
    lcd_->setTextSize(1);
    lcd_->setTextDatum(MC_DATUM);
    lcd_->drawString(paused ? "Resume" : "Pause", bx + btnW / 2, by + btnH / 2);
}

void GameDisplay::drawRedPulse(float urgency) {
    // urgency 0.4..1.0 → alpha overlay 0..180 (on a 0-255 scale)
    float t = (urgency - 0.4f) / 0.6f;
    // Draw a semi-transparent red border that pulses in
    uint8_t thickness = (uint8_t)(t * 12);
    if (thickness == 0) return;
    uint16_t pulseColor = Colors::Red;
    // Top and bottom bars
    lcd_->fillRect(0, kHeaderH, W, thickness, pulseColor);
    lcd_->fillRect(0, H - kFooterH - thickness, W, thickness, pulseColor);
    // Left and right bars
    lcd_->fillRect(0, kHeaderH, thickness, kWordH, pulseColor);
    lcd_->fillRect(W - thickness, kHeaderH, thickness, kWordH, pulseColor);
}
