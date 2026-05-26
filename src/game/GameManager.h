#pragma once
#include <Arduino.h>
#include "display/Colors.h"

class GameManager {
public:
    // Configuration set during setup
    int      targetScore   = 7;
    uint16_t teamColor[2]  = { Colors::Red, Colors::Blue };

    // Round state
    int      score[2]      = { 0, 0 };

    // ---- Called once per new match ----
    void resetMatch();

    // ---- Called at start of each round (after category + countdown) ----
    void startRound(uint32_t nowMs);

    // ---- Called every loop iteration during Playing state ----
    // Returns true when the bomb explodes.
    bool update(uint32_t nowMs);

    // ---- Called when player taps to pass the word ----
    // Returns the new word.
    String passWord(uint32_t nowMs, const String& nextWord);

    // ---- Urgency factor 0..1 for display pulsing ----
    float urgency() const;

    // ---- Tick timing ----
    // Returns true if a tick sound should play this frame.
    bool consumeTick(uint32_t nowMs);

    // ---- Score ----
    // Returns true if the game is over.
    bool awardPoint(int team);
    int  winner() const;  // -1 if no winner yet

    // ---- Current word ----
    const String& currentWord() const { return currentWord_; }

private:
    uint32_t roundStartMs_   = 0;
    uint32_t timerDurationMs_ = 45000;  // randomised per round 30-60s
    uint32_t lastTickMs_     = 0;
    String   currentWord_    = "";

    float elapsedFraction() const;
    uint32_t tickIntervalMs() const;
};
