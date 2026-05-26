#pragma once
#include <Arduino.h>

enum class SoundId {
    Tick,
    CountdownBeep,
    CountdownGo,
    WordPass,
    Explosion,
    VictoryFanfare,
};

class AudioManager {
public:
    bool begin();
    void play(SoundId id);

private:
    bool codecOk_ = false;

    bool initCodec();
    bool initI2S();
    void writeI2S(const int16_t* samples, size_t count);

    // Tone generators
    void playTone(float freqHz, uint32_t durationMs, float amplitude = 0.4f);
    void playNoise(uint32_t durationMs, float amplitude = 0.3f);
    void playMelody(const float* freqs, const uint32_t* durations, int count);

    static bool esWrite(uint8_t reg, uint8_t val);
    static uint8_t esRead(uint8_t reg);
};
