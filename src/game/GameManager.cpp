#include "GameManager.h"
#include <math.h>

void GameManager::resetMatch() {
    score[0] = 0;
    score[1] = 0;
}

void GameManager::startRound(uint32_t nowMs) {
    roundStartMs_    = nowMs;
    lastTickMs_      = nowMs;
    // Random duration: 30..60 seconds
    timerDurationMs_ = 30000 + random(30000);
}

float GameManager::elapsedFraction() const {
    uint32_t elapsed = millis() - roundStartMs_;
    if (elapsed >= timerDurationMs_) return 1.0f;
    return (float)elapsed / (float)timerDurationMs_;
}

float GameManager::urgency() const {
    return elapsedFraction();
}

// Tick interval accelerates exponentially: starts ~1000ms, ends ~40ms
uint32_t GameManager::tickIntervalMs() const {
    float f = elapsedFraction();
    // interval = 1000 * exp(-3.2 * f) → ranges 1000ms..40ms
    float interval = 1000.0f * expf(-3.2f * f);
    if (interval < 40.f) interval = 40.f;
    return (uint32_t)interval;
}

bool GameManager::consumeTick(uint32_t nowMs) {
    if (nowMs - lastTickMs_ >= tickIntervalMs()) {
        lastTickMs_ = nowMs;
        return true;
    }
    return false;
}

bool GameManager::update(uint32_t nowMs) {
    return (nowMs - roundStartMs_) >= timerDurationMs_;
}

String GameManager::passWord(uint32_t nowMs, const String& nextWord) {
    (void)nowMs;
    currentWord_ = nextWord;
    return currentWord_;
}

bool GameManager::awardPoint(int team) {
    if (team < 0 || team > 1) return false;
    score[team]++;
    return score[team] >= targetScore;
}

int GameManager::winner() const {
    if (score[0] >= targetScore) return 0;
    if (score[1] >= targetScore) return 1;
    return -1;
}
