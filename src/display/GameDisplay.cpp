#include "GameDisplay.h"
#include "axs15231b.h"
#include "Font8x8.h"
#include "board/BoardConfig.h"
#include <Arduino.h>
#include <math.h>
#include <esp_heap_caps.h>
#include <stdio.h>
#include <string.h>

static constexpr int W = kDisplayW;
static constexpr int H = kDisplayH;
static constexpr int PW = BoardConfig::PANEL_NATIVE_WIDTH;   // 172
static constexpr int PH = BoardConfig::PANEL_NATIVE_HEIGHT;  // 640

// ---------------------------------------------------------------------------
// Framebuffer primitives
// ---------------------------------------------------------------------------

void GameDisplay::begin() {
    // Allocate framebuffer in PSRAM
    fb_ = (uint16_t*)heap_caps_malloc((size_t)W * H * 2, MALLOC_CAP_SPIRAM);
    axs15231bInit();
    axs15231bSetBrightnessPercent(80);
    clear();
    flush();
}

void GameDisplay::clear(uint16_t color) {
    uint32_t count = (uint32_t)W * H;
    for (uint32_t i = 0; i < count; i++) fb_[i] = color;
}

void GameDisplay::fillRect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > W) w = W - x;
    if (y + h > H) h = H - y;
    if (w <= 0 || h <= 0) return;
    for (int row = y; row < y + h; row++) {
        uint16_t* p = fb_ + row * W + x;
        for (int col = 0; col < w; col++) p[col] = color;
    }
}

void GameDisplay::drawChar(int x, int y, char c, uint16_t color, int scale) {
    if (c < 32 || c > 127) c = '?';
    const uint8_t* glyph = kFont8x8[(uint8_t)c - 32];
    for (int row = 0; row < 8; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                fillRect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

void GameDisplay::drawString(const char* s, int x, int y, uint16_t color, int scale,
                              bool centerX, bool centerY) {
    if (!s) return;
    int pw = stringPixelWidth(s, scale);
    int ph = 8 * scale;
    if (centerX) x -= pw / 2;
    if (centerY) y -= ph / 2;
    int cx = x;
    for (; *s; s++) {
        drawChar(cx, y, *s, color, scale);
        cx += 8 * scale;
    }
}

int GameDisplay::stringPixelWidth(const char* s, int scale) const {
    return (int)strlen(s) * 8 * scale;
}

int GameDisplay::fitTextScale(const char* text, int maxW, int maxH) const {
    int len = (int)strlen(text);
    for (int scale = 7; scale >= 1; scale--) {
        if (len * 8 * scale <= maxW && 8 * scale <= maxH) return scale;
    }
    return 1;
}

// Push the logical 640×172 framebuffer to the 172×640 physical panel.
// Rotation: logical(lc, lr) → physical(pr=lc, pc=171-lr)
void GameDisplay::flush() {
    // Flush in horizontal strips to limit stack/DMA buffer size
    static uint16_t strip[PW * 32];  // 32 logical rows = 32 physical columns
    const int rowsPerChunk = 32;

    bool firstChunk = true;
    for (int startLr = 0; startLr < H; startLr += rowsPerChunk) {
        int chunkRows = rowsPerChunk;
        if (startLr + chunkRows > H) chunkRows = H - startLr;

        // Physical strip: columns pr = [startLr .. startLr+chunkRows-1]
        // Physical rows  pc = [0 .. 171]
        // strip index: pc * chunkRows + (pr - startLr)
        for (int pc = 0; pc < PW; pc++) {
            int lr = PW - 1 - pc;  // logical row
            for (int pr = startLr; pr < startLr + chunkRows; pr++) {
                int lc = pr;  // logical col
                strip[pc * chunkRows + (pr - startLr)] =
                    fb_[(uint32_t)lr * W + lc];
            }
        }

        uint16_t physX = startLr;                    // physical x = logical col start
        uint16_t physY = firstChunk ? 0 : 1;        // 0 = new write, 1 = continue
        axs15231bPushColors(physX, physY, chunkRows, PW, strip);
        firstChunk = false;
    }
}

// ---------------------------------------------------------------------------
// Screen implementations
// ---------------------------------------------------------------------------

void GameDisplay::drawSetupScore(int selectedIndex, const int* options, int count) {
    clear();
    const char* title = "First to how many?";
    drawString(title, W / 2, 20, Colors::White, 2, true, true);

    int btnW = 80, btnH = 42, gap = 18;
    int totalW = count * btnW + (count - 1) * gap;
    int startX = (W - totalW) / 2;

    for (int i = 0; i < count; i++) {
        int bx = startX + i * (btnW + gap);
        int by = 70;
        uint16_t bg = (i == selectedIndex) ? Colors::Orange : Colors::DimGray;
        fillRect(bx, by, btnW, btnH, bg);

        char buf[8];
        snprintf(buf, sizeof(buf), "%d", options[i]);
        drawString(buf, bx + btnW / 2, by + btnH / 2, Colors::White, 3, true, true);
    }

    drawString("Tap a number, then tap again to confirm",
               W / 2, H - 16, Colors::Gray, 1, true, true);
    flush();
}

void GameDisplay::drawCategorySelect(const char* categoryName, int index, int total,
                                      bool sdOk) {
    clear();
    if (!sdOk) {
        drawString("No SD card found!", W / 2, H / 2 - 10, Colors::Red, 2, true, true);
        drawString("Insert SD with /categories/*.txt", W / 2, H / 2 + 16, Colors::Gray, 1, true, true);
        flush();
        return;
    }

    drawString("Choose Category", W / 2, 14, Colors::Gray, 1, true, true);
    drawString("<", 22, H / 2, Colors::White, 3, true, true);
    drawString(">", W - 22, H / 2, Colors::White, 3, true, true);

    int scale = fitTextScale(categoryName, W - 120, 40);
    drawString(categoryName, W / 2, H / 2, Colors::Yellow, scale, true, true);

    char buf[24];
    snprintf(buf, sizeof(buf), "%d / %d", index + 1, total);
    drawString(buf, W / 2, H - 28, Colors::Gray, 1, true, true);
    drawString("Tap center to start  |  Hold POWER 3s = RSVP Nano",
               W / 2, H - 14, Colors::Gray, 1, true, true);
    flush();
}

void GameDisplay::drawTeamColorSetup(int team, int selectedColor, uint16_t otherTeamColor) {
    clear();
    char title[32];
    snprintf(title, sizeof(title), "Team %d color", team + 1);
    drawString(title, W / 2, 18, Colors::White, 2, true, true);

    int swatchSize = 44, gap = 20;
    int totalW = kTeamColorCount * swatchSize + (kTeamColorCount - 1) * gap;
    int startX = (W - totalW) / 2;

    for (int i = 0; i < kTeamColorCount; i++) {
        int bx = startX + i * (swatchSize + gap);
        int by = 60;
        uint16_t c = kTeamColorOptions[i];
        fillRect(bx, by, swatchSize, swatchSize, c);
        if (i == selectedColor) {
            // White border (2px)
            fillRect(bx - 2, by - 2, swatchSize + 4, 2, Colors::White);
            fillRect(bx - 2, by + swatchSize, swatchSize + 4, 2, Colors::White);
            fillRect(bx - 2, by - 2, 2, swatchSize + 4, Colors::White);
            fillRect(bx + swatchSize, by - 2, 2, swatchSize + 4, Colors::White);
        }
        if (c == otherTeamColor) {
            // Gray X to indicate conflict
            fillRect(bx, by, swatchSize, 2, Colors::Gray);
            fillRect(bx, by + swatchSize - 2, swatchSize, 2, Colors::Gray);
        }
    }

    drawString("Tap a color. Tap selected to confirm.",
               W / 2, H - 16, Colors::Gray, 1, true, true);
    flush();
}

void GameDisplay::drawCountdown(int number) {
    clear();
    if (number > 0) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", number);
        drawString(buf, W / 2, H / 2, Colors::White, 8, true, true);
    } else {
        drawString("GO!", W / 2, H / 2, Colors::Green, 6, true, true);
    }
    flush();
}

void GameDisplay::drawPlaying(const char* word, const char* category,
                               int scoreLeft, int scoreRight,
                               uint16_t colorLeft, uint16_t colorRight,
                               float urgency, bool paused) {
    clear();

    // Header bar
    fillRect(0, 0, W, kHeaderH, Colors::DimGray);
    drawScoreBox(0, 110, scoreLeft, colorLeft);
    drawScoreBox(W - 110, 110, scoreRight, colorRight);
    drawString(category, W / 2, kHeaderH / 2, Colors::LightGray, 1, true, true);

    // Word
    drawWord(word, kWordY, kWordH, Colors::White);

    // Urgency pulse
    if (urgency > 0.4f) drawRedPulse(urgency);

    // Footer pause button
    drawPauseButton(paused);

    if (paused) {
        fillRect(0, 0, W, H, Colors::Black);
        drawString("PAUSED", W / 2, H / 2 - 16, Colors::White, 4, true, true);
        drawString("Tap to resume", W / 2, H / 2 + 20, Colors::Gray, 1, true, true);
    }

    flush();
}

void GameDisplay::drawExplosion() {
    clear(Colors::Red);
    drawString("BOOM!", W / 2, H / 2, Colors::White, 6, true, true);
    flush();
}

void GameDisplay::drawScorePrompt(uint16_t colorLeft, uint16_t colorRight,
                                   int scoreLeft, int scoreRight) {
    clear();
    fillRect(0, 0, W / 2 - 2, H, colorLeft);
    fillRect(W / 2 - 2, 0, 4, H, Colors::White);
    fillRect(W / 2 + 2, 0, W / 2 - 2, H, colorRight);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", scoreLeft);
    drawString(buf, W / 4, H / 2 - 20, Colors::White, 4, true, true);
    snprintf(buf, sizeof(buf), "%d", scoreRight);
    drawString(buf, 3 * W / 4, H / 2 - 20, Colors::White, 4, true, true);

    drawString("Tap your side!", W / 2, H - 20, Colors::White, 2, true, true);
    flush();
}

void GameDisplay::drawVictory(int winningTeam, uint16_t winColor, int score, int targetScore) {
    clear();

    // Confetti
    for (int i = 0; i < kConfettiCount; i++) {
        Confetti& c = confetti_[i];
        int cx = (int)c.x, cy = (int)c.y;
        fillRect(cx, cy, c.size, c.size, c.color);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "Team %d Wins!", winningTeam + 1);
    drawString(buf, W / 2, 38, winColor, 3, true, true);

    snprintf(buf, sizeof(buf), "%d - %d", score, targetScore);
    drawString(buf, W / 2, 80, Colors::White, 2, true, true);

    drawString("Tap to play again", W / 2, H - 16, Colors::Gray, 1, true, true);
    flush();
}

void GameDisplay::flashScreen(uint16_t color, int ms) {
    clear(color);
    flush();
    delay(ms);
}

// ---------------------------------------------------------------------------
// Confetti
// ---------------------------------------------------------------------------
void GameDisplay::confettiInit() {
    for (int i = 0; i < kConfettiCount; i++) {
        confetti_[i].x     = random(W);
        confetti_[i].y     = random(H);
        confetti_[i].dx    = (random(40) - 20) / 10.0f;
        confetti_[i].dy    = (random(20) + 5) / 10.0f;
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
        if (confetti_[i].y > H)  { confetti_[i].y = -(float)confetti_[i].size; confetti_[i].x = random(W); }
        if (confetti_[i].x < 0)  confetti_[i].x = W;
        if (confetti_[i].x > W)  confetti_[i].x = 0;
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void GameDisplay::drawWord(const char* text, int y, int h, uint16_t color) {
    int scale = fitTextScale(text, W - 20, h - 10);
    drawString(text, W / 2, y + h / 2, color, scale, true, true);
}

void GameDisplay::drawScoreBox(int x, int w, int score, uint16_t color) {
    fillRect(x, 0, w, kHeaderH, color);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", score);
    drawString(buf, x + w / 2, kHeaderH / 2, Colors::White, 3, true, true);
}

void GameDisplay::drawRedPulse(float urgency) {
    float t = (urgency - 0.4f) / 0.6f;
    int thickness = (int)(t * 12);
    if (thickness == 0) return;
    // Top / bottom bars inside the word area
    fillRect(0, kHeaderH, W, thickness, Colors::Red);
    fillRect(0, H - kFooterH - thickness, W, thickness, Colors::Red);
    // Left / right bars
    fillRect(0, kHeaderH, thickness, kWordH, Colors::Red);
    fillRect(W - thickness, kHeaderH, thickness, kWordH, Colors::Red);
}

void GameDisplay::drawPauseButton(bool paused) {
    int btnW = 70, btnH = 20;
    int bx = (W - btnW) / 2;
    int by = H - kFooterH + 4;
    fillRect(bx, by, btnW, btnH, Colors::DimGray);
    drawString(paused ? "Resume" : "Pause",
               bx + btnW / 2, by + btnH / 2,
               Colors::LightGray, 1, true, true);
}
