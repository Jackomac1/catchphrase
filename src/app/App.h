#pragma once
#include "AppState.h"
#include "display/LCD.h"
#include "display/GameDisplay.h"
#include "input/ButtonHandler.h"
#include "audio/AudioManager.h"
#include "storage/CategoryLoader.h"
#include "game/GameManager.h"

class App {
public:
    void begin();
    void update(uint32_t nowMs);

private:
    LGFX            lcd_;
    GameDisplay     display_;
    ButtonHandler   btnPower_  { BoardConfig::PIN_POWER };
    ButtonHandler   btnBoot_   { BoardConfig::PIN_BOOT  };
    AudioManager    audio_;
    CategoryLoader  cats_;
    GameManager     game_;

    AppState        state_      = AppState::SetupScore;

    // Setup state
    static constexpr int kScoreOptions[]  = { 3, 5, 7, 10, 15 };
    static constexpr int kScoreCount      = 5;
    int  scoreOptIndex_    = 2;   // default index → 7
    int  colorSetupIndex_  = 0;   // which color swatch is highlighted during setup
    int  catIndex_         = 0;   // selected category index

    // Countdown
    int      countdownNum_  = 3;
    uint32_t countdownMs_   = 0;

    // Touch tracking
    bool     lastTouched_   = false;
    int      lastTouchX_    = 0;
    int      lastTouchY_    = 0;

    // Dirty flag — only redraw when state or data changes
    bool     dirty_         = true;
    uint32_t stateEnteredMs_ = 0;

    void setState(AppState next, uint32_t nowMs);
    bool getTouchTap(int& x, int& y);  // returns true on a fresh tap

    // Per-state update methods
    void updateSetupScore(uint32_t nowMs);
    void updateColorSetup(AppState thisState, int team, uint32_t nowMs);
    void updateCategorySelect(uint32_t nowMs);
    void updateCountdown(uint32_t nowMs);
    void updatePlaying(uint32_t nowMs);
    void updateExploded(uint32_t nowMs);
    void updateScorePrompt(uint32_t nowMs);
    void updateVictory(uint32_t nowMs);
    void switchToRsvpNano();  // activate ota_1 and reboot into RSVP Nano
};
