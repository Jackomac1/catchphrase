#include "AudioManager.h"
#include "board/BoardConfig.h"
#include <Wire.h>
#include <driver/i2s.h>
#include <math.h>

static constexpr i2s_port_t  kI2SPort    = I2S_NUM_0;
static constexpr int         kSampleRate = 44100;
static constexpr int         kDmaBufs    = 4;
static constexpr int         kDmaBufLen  = 256;  // samples per DMA buffer

// ---- ES8311 register helpers ------------------------------------------------

bool AudioManager::esWrite(uint8_t reg, uint8_t val) {
    Wire1.beginTransmission(BoardConfig::ES8311_ADDRESS);
    Wire1.write(reg);
    Wire1.write(val);
    return Wire1.endTransmission() == 0;
}

uint8_t AudioManager::esRead(uint8_t reg) {
    Wire1.beginTransmission(BoardConfig::ES8311_ADDRESS);
    Wire1.write(reg);
    Wire1.endTransmission(false);
    Wire1.requestFrom((uint8_t)BoardConfig::ES8311_ADDRESS, (uint8_t)1);
    return Wire1.available() ? Wire1.read() : 0xFF;
}

// ---- ES8311 init (44100 Hz, 16-bit I2S, MCLK = 256*fs) --------------------

bool AudioManager::initCodec() {
    // Verify chip ID
    uint8_t id0 = esRead(0xFD);
    uint8_t id1 = esRead(0xFE);
    if (id0 == 0xFF && id1 == 0xFF) return false;  // not found

    esWrite(0x00, 0x3F);  // soft reset all
    delay(10);
    esWrite(0x00, 0x00);  // release reset

    // Clock configuration for 44100 Hz, MCLK = 256*fs = 11.289 MHz
    esWrite(0x01, 0x3A);  // MCLK from I2S, SEQ_EN, DIV_RATIO
    esWrite(0x02, 0x00);  // MCLK prescaler /1
    esWrite(0x03, 0x20);  // BCLK divider
    esWrite(0x04, 0x20);  // ADC LRCK
    esWrite(0x05, 0x00);  // DAC OSR: 256
    esWrite(0x06, 0x03);  // DAC LRCK
    esWrite(0x07, 0x00);
    esWrite(0x08, 0xFF);  // enable all clocks

    // I2S interface: standard I2S, 16-bit
    esWrite(0x09, 0x0C);  // ADC interface
    esWrite(0x0A, 0x0C);  // DAC interface

    // Power management
    esWrite(0x0D, 0x01);  // enable ADC/DAC power
    esWrite(0x0F, 0x00);  // ADC: MIC1P/MIC1N differential
    esWrite(0x12, 0x00);  // ADC digital power on

    // DAC setup
    esWrite(0x37, 0x08);  // DAC volume: 0 dB
    esWrite(0x32, 0x00);  // unmute DAC

    // Analog output
    esWrite(0x45, 0x00);

    BoardConfig::enableAudioAmp();
    return true;
}

// ---- I2S peripheral init ---------------------------------------------------

bool AudioManager::initI2S() {
    i2s_config_t cfg = {
        .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate          = kSampleRate,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = kDmaBufs,
        .dma_buf_len          = kDmaBufLen,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
    };

    if (i2s_driver_install(kI2SPort, &cfg, 0, nullptr) != ESP_OK) return false;

    i2s_pin_config_t pins = {
        .mck_io_num   = BoardConfig::PIN_I2S_MCLK,
        .bck_io_num   = BoardConfig::PIN_I2S_BCLK,
        .ws_io_num    = BoardConfig::PIN_I2S_WS,
        .data_out_num = BoardConfig::PIN_I2S_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE,
    };

    return i2s_set_pin(kI2SPort, &pins) == ESP_OK;
}

// ---- Public API ------------------------------------------------------------

bool AudioManager::begin() {
    codecOk_ = initCodec();
    if (!codecOk_) return false;
    return initI2S();
}

// ---- Tone generation -------------------------------------------------------

void AudioManager::writeI2S(const int16_t* samples, size_t count) {
    // Interleave mono into stereo (L+R identical)
    static int16_t stereo[kDmaBufLen * 2];
    size_t offset = 0;
    while (offset < count) {
        size_t chunk = min(count - offset, (size_t)kDmaBufLen);
        for (size_t i = 0; i < chunk; i++) {
            stereo[i * 2]     = samples[offset + i];
            stereo[i * 2 + 1] = samples[offset + i];
        }
        size_t written = 0;
        i2s_write(kI2SPort, stereo, chunk * 4, &written, portMAX_DELAY);
        offset += chunk;
    }
}

void AudioManager::playTone(float freqHz, uint32_t durationMs, float amplitude) {
    const int totalSamples = (kSampleRate * durationMs) / 1000;
    const int attackSamples  = kSampleRate * 5 / 1000;   // 5ms attack
    const int releaseSamples = kSampleRate * 10 / 1000;  // 10ms release
    const int16_t amp = (int16_t)(amplitude * 32767.f);

    static int16_t buf[kDmaBufLen];
    int written = 0;
    while (written < totalSamples) {
        int chunk = min(kDmaBufLen, totalSamples - written);
        for (int i = 0; i < chunk; i++) {
            int s = written + i;
            float env = 1.0f;
            if (s < attackSamples)
                env = (float)s / attackSamples;
            else if (s > totalSamples - releaseSamples)
                env = (float)(totalSamples - s) / releaseSamples;

            // Square wave
            float phase = fmodf((float)s * freqHz / kSampleRate, 1.0f);
            int16_t val = (phase < 0.5f) ? amp : -amp;
            buf[i] = (int16_t)(val * env);
        }
        writeI2S(buf, chunk);
        written += chunk;
    }
}

void AudioManager::playNoise(uint32_t durationMs, float amplitude) {
    const int totalSamples = (kSampleRate * durationMs) / 1000;
    const int16_t amp = (int16_t)(amplitude * 32767.f);

    static int16_t buf[kDmaBufLen];
    int written = 0;
    while (written < totalSamples) {
        int chunk = min(kDmaBufLen, totalSamples - written);
        for (int i = 0; i < chunk; i++) {
            int s = written + i;
            float env = 1.0f - (float)s / totalSamples;  // decay
            // White noise + low-frequency rumble
            int32_t noise = (rand() % 65535) - 32768;
            float rumble = sinf(2 * M_PI * 60.f * s / kSampleRate) * 0.5f * 32767.f;
            buf[i] = (int16_t)(((noise * 0.7f) + rumble) * amp / 32768.f * env);
        }
        writeI2S(buf, chunk);
        written += chunk;
    }
}

void AudioManager::playMelody(const float* freqs, const uint32_t* durations, int count) {
    for (int i = 0; i < count; i++) {
        playTone(freqs[i], durations[i], 0.5f);
        delay(20);  // brief gap between notes
    }
}

void AudioManager::play(SoundId id) {
    if (!codecOk_) return;
    switch (id) {
        case SoundId::Tick: {
            playTone(880.f, 30, 0.25f);
            break;
        }
        case SoundId::CountdownBeep: {
            playTone(1047.f, 150, 0.45f);  // C6
            break;
        }
        case SoundId::CountdownGo: {
            static const float f[]  = {523.f, 659.f, 784.f};  // C5-E5-G5
            static const uint32_t d[] = {100, 100, 200};
            playMelody(f, d, 3);
            break;
        }
        case SoundId::WordPass: {
            playTone(600.f, 18, 0.15f);
            break;
        }
        case SoundId::Explosion: {
            playNoise(700, 0.8f);
            break;
        }
        case SoundId::VictoryFanfare: {
            static const float f[]    = {262.f, 330.f, 392.f, 523.f};
            static const uint32_t d[] = {150, 150, 150, 400};
            playMelody(f, d, 4);
            break;
        }
    }
}
