#include "App.h"
#include "board/BoardConfig.h"
#include <esp_ota_ops.h>
#include <esp_partition.h>

constexpr int App::kScoreOptions[];

void App::begin() {
    BoardConfig::begin();
    display_.begin(lcd_);
    btnPower_.begin();
    btnBoot_.begin();
    audio_.begin();
    bool sdOk = cats_.begin();
    if (!sdOk) catIndex_ = -1;

    game_.teamColor[0] = Colors::Red;
    game_.teamColor[1] = Colors::Blue;

    state_ = AppState::SetupScore;
    dirty_ = true;
}

void App::update(uint32_t nowMs) {
    btnPower_.update(nowMs);
    btnBoot_.update(nowMs);

    switch (state_) {
        case AppState::SetupScore:      updateSetupScore(nowMs);                       break;
        case AppState::SetupTeam0Color: updateColorSetup(AppState::SetupTeam0Color, 0, nowMs); break;
        case AppState::SetupTeam1Color: updateColorSetup(AppState::SetupTeam1Color, 1, nowMs); break;
        case AppState::CategorySelect:  updateCategorySelect(nowMs);                   break;
        case AppState::Countdown:       updateCountdown(nowMs);                        break;
        case AppState::Playing:
        case AppState::Paused:          updatePlaying(nowMs);                          break;
        case AppState::Exploded:        updateExploded(nowMs);                         break;
        case AppState::ScorePrompt:     updateScorePrompt(nowMs);                      break;
        case AppState::Victory:         updateVictory(nowMs);                          break;
    }
}

void App::setState(AppState next, uint32_t nowMs) {
    state_          = next;
    stateEnteredMs_ = nowMs;
    dirty_          = true;
}

// Returns true on a new tap (finger-down edge).
bool App::getTouchTap(int& x, int& y) {
    int16_t tx, ty;
    bool touched = lcd_.getTouch(&tx, &ty);
    bool isTap   = touched && !lastTouched_;
    lastTouched_ = touched;
    if (isTap) {
        x = tx; y = ty;
        lastTouchX_ = tx; lastTouchY_ = ty;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// SetupScore
// ---------------------------------------------------------------------------
void App::updateSetupScore(uint32_t nowMs) {
    if (dirty_) {
        display_.drawSetupScore(scoreOptIndex_, kScoreOptions, kScoreCount);
        dirty_ = false;
    }

    int tx, ty;
    if (getTouchTap(tx, ty)) {
        // Map tap X to option index
        int btnW = 80, gap = 18;
        int totalW = kScoreCount * btnW + (kScoreCount - 1) * gap;
        int startX = (640 - totalW) / 2;
        for (int i = 0; i < kScoreCount; i++) {
            int bx = startX + i * (btnW + gap);
            if (tx >= bx && tx < bx + btnW && ty >= 70 && ty < 70 + 42) {
                if (i == scoreOptIndex_) {
                    // Second tap on same: confirm
                    game_.targetScore = kScoreOptions[scoreOptIndex_];
                    setState(AppState::SetupTeam0Color, nowMs);
                } else {
                    scoreOptIndex_ = i;
                    dirty_ = true;
                }
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Color setup (reused for both teams)
// ---------------------------------------------------------------------------
void App::updateColorSetup(AppState thisState, int team, uint32_t nowMs) {
    if (dirty_) {
        uint16_t other = game_.teamColor[1 - team];
        display_.drawTeamColorSetup(team, colorSetupIndex_, other);
        dirty_ = false;
    }

    int tx, ty;
    if (getTouchTap(tx, ty)) {
        int swatchSize = 44, gap = 20;
        int totalW = kTeamColorCount * swatchSize + (kTeamColorCount - 1) * gap;
        int startX = (640 - totalW) / 2;

        for (int i = 0; i < kTeamColorCount; i++) {
            int bx = startX + i * (swatchSize + gap);
            if (tx >= bx && tx < bx + swatchSize && ty >= 60 && ty < 60 + swatchSize) {
                if (i == colorSetupIndex_) {
                    // Confirm selection if not same as other team
                    uint16_t chosen = kTeamColorOptions[i];
                    if (chosen == game_.teamColor[1 - team]) return;  // conflict
                    game_.teamColor[team] = chosen;
                    colorSetupIndex_ = 0;
                    AppState next = (team == 0) ? AppState::SetupTeam1Color
                                               : AppState::CategorySelect;
                    setState(next, nowMs);
                } else {
                    colorSetupIndex_ = i;
                    dirty_ = true;
                }
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Category select
// ---------------------------------------------------------------------------
void App::updateCategorySelect(uint32_t nowMs) {
    bool sdOk = !cats_.categories().empty();

    // Power button held 3 s on this screen → switch to RSVP Nano
    if (btnPower_.heldDurationMs(nowMs) > 3000) {
        switchToRsvpNano();
        return;
    }

    if (dirty_) {
        const char* name = sdOk ? cats_.categories()[catIndex_].name.c_str() : "";
        display_.drawCategorySelect(name, catIndex_, (int)cats_.categories().size(), sdOk);
        dirty_ = false;
    }

    if (!sdOk) return;

    int tx, ty;
    if (getTouchTap(tx, ty)) {
        int n = (int)cats_.categories().size();
        if (tx < 80) {
            // Left arrow
            catIndex_ = (catIndex_ + n - 1) % n;
            dirty_ = true;
        } else if (tx > 560) {
            // Right arrow
            catIndex_ = (catIndex_ + 1) % n;
            dirty_ = true;
        } else {
            // Center tap: load and start
            cats_.loadWords(cats_.categories()[catIndex_]);
            game_.resetMatch();
            game_.passWord(nowMs, cats_.nextWord());
            countdownNum_ = 3;
            setState(AppState::Countdown, nowMs);
        }
    }
}

// ---------------------------------------------------------------------------
// Countdown
// ---------------------------------------------------------------------------
void App::updateCountdown(uint32_t nowMs) {
    if (dirty_) {
        display_.drawCountdown(countdownNum_);
        if (countdownNum_ > 0)
            audio_.play(SoundId::CountdownBeep);
        else
            audio_.play(SoundId::CountdownGo);
        dirty_ = false;
    }

    if (nowMs - stateEnteredMs_ >= 900) {
        countdownNum_--;
        if (countdownNum_ < 0) {
            // Launch round
            game_.startRound(nowMs);
            setState(AppState::Playing, nowMs);
        } else {
            stateEnteredMs_ = nowMs;
            dirty_ = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Playing (and Paused)
// ---------------------------------------------------------------------------
void App::updatePlaying(uint32_t nowMs) {
    bool paused = (state_ == AppState::Paused);

    // Power button toggles pause
    if (btnPower_.wasPressedEvent()) {
        setState(paused ? AppState::Playing : AppState::Paused, nowMs);
        paused = !paused;
        dirty_ = true;
    }

    if (!paused) {
        // Tick sound
        if (game_.consumeTick(nowMs)) {
            audio_.play(SoundId::Tick);
        }

        // Check explosion
        if (game_.update(nowMs)) {
            audio_.play(SoundId::Explosion);
            display_.flashScreen(Colors::Red, 200);
            display_.drawExplosion();
            delay(800);
            setState(AppState::ScorePrompt, nowMs);
            return;
        }

        // Touch: tap center word → pass
        int tx, ty;
        if (getTouchTap(tx, ty)) {
            // Pause button area
            if (ty >= 172 - 28 && tx >= 285 && tx <= 355) {
                setState(AppState::Paused, nowMs);
                dirty_ = true;
                return;
            }
            // Word area tap → pass
            audio_.play(SoundId::WordPass);
            game_.passWord(nowMs, cats_.nextWord());
            display_.flashScreen(Colors::White, 60);
            dirty_ = true;
        }
    } else {
        // Paused: any touch resumes
        int tx, ty;
        if (getTouchTap(tx, ty)) {
            setState(AppState::Playing, nowMs);
            dirty_ = true;
            return;
        }
    }

    // Redraw if word or urgency changed (redraw every ~100ms for pulse animation)
    static uint32_t lastDraw = 0;
    if (!paused && (nowMs - lastDraw > 80 || dirty_)) {
        lastDraw = nowMs;
        display_.drawPlaying(
            game_.currentWord().c_str(),
            cats_.categories()[catIndex_].name.c_str(),
            game_.score[0], game_.score[1],
            game_.teamColor[0], game_.teamColor[1],
            game_.urgency(),
            false
        );
        dirty_ = false;
    } else if (paused && dirty_) {
        display_.drawPlaying(
            game_.currentWord().c_str(),
            cats_.categories()[catIndex_].name.c_str(),
            game_.score[0], game_.score[1],
            game_.teamColor[0], game_.teamColor[1],
            game_.urgency(),
            true
        );
        dirty_ = false;
    }
}

// ---------------------------------------------------------------------------
// Exploded: brief display before score prompt
// ---------------------------------------------------------------------------
void App::updateExploded(uint32_t nowMs) {
    // (explosion is handled inline in updatePlaying → jumps to ScorePrompt directly)
    setState(AppState::ScorePrompt, nowMs);
}

// ---------------------------------------------------------------------------
// Score prompt: tap left or right half
// ---------------------------------------------------------------------------
void App::updateScorePrompt(uint32_t nowMs) {
    if (dirty_) {
        display_.drawScorePrompt(game_.teamColor[0], game_.teamColor[1],
                                 game_.score[0], game_.score[1]);
        dirty_ = false;
    }

    int tx, ty;
    if (getTouchTap(tx, ty)) {
        int team = (tx < 320) ? 0 : 1;
        bool won = game_.awardPoint(team);
        if (won) {
            display_.confettiInit();
            audio_.play(SoundId::VictoryFanfare);
            setState(AppState::Victory, nowMs);
        } else {
            // Next round
            game_.passWord(nowMs, cats_.nextWord());
            countdownNum_ = 3;
            setState(AppState::Countdown, nowMs);
        }
    }
}

// ---------------------------------------------------------------------------
// Victory
// ---------------------------------------------------------------------------
void App::updateVictory(uint32_t nowMs) {
    display_.confettiTick(nowMs);
    int w = game_.winner();
    display_.drawVictory(w, game_.teamColor[w],
                         game_.score[w], game_.targetScore);

    int tx, ty;
    if (getTouchTap(tx, ty)) {
        // Restart to category select
        game_.resetMatch();
        setState(AppState::CategorySelect, nowMs);
    }
}

// ---------------------------------------------------------------------------
// Switch to RSVP Nano (ota_1) and reboot.
// On the NEXT hard power cycle after rsvpnano runs, hold BOOT at power-on
// to return to RSVP Nano, or release to come back to Catchphrase.
// ---------------------------------------------------------------------------
void App::switchToRsvpNano() {
    lcd_.fillScreen(0x0000);
    lcd_.setTextDatum(MC_DATUM);
    lcd_.setTextColor(0xFFFF, 0x0000);
    lcd_.setTextSize(2);
    lcd_.drawString("Switching to RSVP Nano...", 320, 86);
    delay(600);

    const esp_partition_t* ota1 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr);

    if (ota1) {
        esp_ota_set_boot_partition(ota1);
        esp_restart();
    }

    // ota_1 not found — show error and stay in Catchphrase
    lcd_.fillScreen(0x0000);
    lcd_.setTextColor(0xF800, 0x0000);
    lcd_.drawString("RSVP Nano not flashed yet.", 320, 86);
    lcd_.setTextSize(1);
    lcd_.drawString("See README: flash rsvpnano to ota_1 first.", 320, 110);
    delay(3000);
    setState(AppState::CategorySelect, millis());
    dirty_ = true;
}
