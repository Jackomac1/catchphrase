#include "axs15231b.h"
#include "board/BoardConfig.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>
#include <string.h>

static const char* TAG = "AXS15231B";

static spi_device_handle_t spi_;

// ---------------------------------------------------------------------------
// Init command table (from AXS15231B datasheet + rsvpnano)
// ---------------------------------------------------------------------------
struct InitCmd {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t len;   // data byte count; 0xFF = delay (data[0] ms)
};

static const InitCmd kInitCmds[] = {
    {0xBB, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5A, 0xA5}, 8},
    {0xC1, {0x33}, 1},
    {0xC2, {0x23}, 1},
    {0xC3, {0x22, 0x11}, 2},
    {0xC4, {0x05}, 1},
    {0xC5, {0x00, 0x0A, 0x00, 0x10}, 4},
    {0xC6, {0x05, 0x0A, 0x05, 0x0A}, 4},
    {0xC7, {0x01}, 1},
    {0xC8, {0x04}, 1},
    {0xC9, {0x02, 0x0F, 0x01, 0x0F, 0x01, 0x0F, 0x01, 0x0F}, 8},
    {0xCB, {0x80, 0x40, 0x00}, 3},
    {0xCC, {0x01, 0x04, 0x00}, 3},
    {0xD0, {0x00, 0x01, 0x22, 0x02, 0x02}, 5},
    {0xD1, {0x00, 0x01, 0x22, 0x02, 0x02}, 5},
    {0xD2, {0x00, 0x01, 0x22, 0x02, 0x02}, 5},
    {0xD3, {0x00, 0x01, 0x22, 0x02, 0x02}, 5},
    {0xE0, {0x70, 0x00, 0x01, 0x02, 0x04, 0x05, 0x08, 0x0C,
            0x10, 0x22, 0x2E, 0x38, 0x3E, 0x46, 0x4C, 0x52}, 16},
    {0xE1, {0x70, 0x00, 0x01, 0x02, 0x04, 0x05, 0x08, 0x0C,
            0x10, 0x22, 0x2E, 0x38, 0x3E, 0x46, 0x4C, 0x52}, 16},
    {0xE2, {0x70, 0x00, 0x01, 0x02, 0x04, 0x05, 0x08, 0x0C,
            0x10, 0x22, 0x2E, 0x38, 0x3E, 0x46, 0x4C, 0x52}, 16},
    {0x35, {0x00}, 1},
    {0x36, {0x00}, 1},
    {0x3A, {0x55}, 1},  // 16-bit RGB565
    {0x11, {}, 0},      // Sleep out
    {0x00, {}, 0xFF},   // delay 120ms (sentinel: cmd=0, len=0xFF)
    {0x29, {}, 0},      // Display on
};

// ---------------------------------------------------------------------------
// SPI helpers
// ---------------------------------------------------------------------------
static void sendCmd(uint8_t cmd, const uint8_t* data, size_t len) {
    spi_transaction_ext_t ext = {};
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR |
                     SPI_TRANS_VARIABLE_DUMMY;
    ext.command_bits = 8;
    ext.address_bits = 24;
    ext.base.cmd  = 0x02;
    ext.base.addr = (uint32_t)cmd << 8;

    if (len == 0) {
        spi_device_polling_transmit(spi_, (spi_transaction_t*)&ext);
        return;
    }

    ext.base.tx_buffer = data;
    ext.base.length    = len * 8;
    spi_device_polling_transmit(spi_, (spi_transaction_t*)&ext);
}

static void setColumnRow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint8_t ca[4] = {(uint8_t)(x >> 8), (uint8_t)x,
                     (uint8_t)((x + w - 1) >> 8), (uint8_t)(x + w - 1)};
    uint8_t ra[4] = {(uint8_t)(y >> 8), (uint8_t)y,
                     (uint8_t)((y + h - 1) >> 8), (uint8_t)(y + h - 1)};
    sendCmd(0x2A, ca, 4);
    sendCmd(0x2B, ra, 4);
}

// ---------------------------------------------------------------------------
// LEDC backlight
// ---------------------------------------------------------------------------
static void initBacklight() {
    ledc_timer_config_t timer = {};
    timer.speed_mode      = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_8_BIT;
    timer.timer_num       = LEDC_TIMER_0;
    timer.freq_hz         = 1000;
    timer.clk_cfg         = LEDC_AUTO_CLK;
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {};
    ch.gpio_num   = BoardConfig::PIN_LCD_BACKLIGHT;
    ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ch.channel    = LEDC_CHANNEL_0;
    ch.timer_sel  = LEDC_TIMER_0;
    ch.duty       = 255;
    ch.hpoint     = 0;
    ledc_channel_config(&ch);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void axs15231bInit() {
    // Reset
    gpio_set_direction((gpio_num_t)BoardConfig::PIN_LCD_RST, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)BoardConfig::PIN_LCD_RST, 0);
    delay(20);
    gpio_set_level((gpio_num_t)BoardConfig::PIN_LCD_RST, 1);
    delay(120);

    // SPI bus
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = BoardConfig::PIN_LCD_SCLK;
    buscfg.data0_io_num = BoardConfig::PIN_LCD_DATA0;
    buscfg.data1_io_num = BoardConfig::PIN_LCD_DATA1;
    buscfg.data2_io_num = BoardConfig::PIN_LCD_DATA2;
    buscfg.data3_io_num = BoardConfig::PIN_LCD_DATA3;
    buscfg.max_transfer_sz = BoardConfig::PANEL_NATIVE_WIDTH *
                             BoardConfig::PANEL_NATIVE_HEIGHT * 2 + 64;
    buscfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;
    spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits     = 0;
    devcfg.address_bits     = 0;
    devcfg.mode             = SPI_MODE3;
    devcfg.clock_speed_hz   = 40 * 1000 * 1000;
    devcfg.spics_io_num     = BoardConfig::PIN_LCD_CS;
    devcfg.queue_size       = 1;
    devcfg.flags            = SPI_DEVICE_HALFDUPLEX;
    spi_bus_add_device(SPI3_HOST, &devcfg, &spi_);

    // Init sequence
    for (size_t i = 0; i < sizeof(kInitCmds) / sizeof(kInitCmds[0]); i++) {
        const InitCmd& c = kInitCmds[i];
        if (c.len == 0xFF) {
            // Delay sentinel — cmd==0 means use 120ms default
            delay(c.cmd == 0 ? 120 : c.data[0]);
        } else {
            sendCmd(c.cmd, c.data, c.len);
        }
    }

    initBacklight();
    ESP_LOGI(TAG, "Panel ready");
}

void axs15231bSetBacklight(bool on) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, on ? 255 : 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void axs15231bSetBrightnessPercent(uint8_t percent) {
    uint32_t duty = (uint32_t)percent * 255 / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void axs15231bSleep() {
    uint8_t d = 0;
    sendCmd(0x28, &d, 0);  // display off
    sendCmd(0x10, &d, 0);  // sleep in
    axs15231bSetBacklight(false);
}

void axs15231bWake() {
    uint8_t d = 0;
    sendCmd(0x11, &d, 0);  // sleep out
    delay(120);
    sendCmd(0x29, &d, 0);  // display on
    axs15231bSetBacklight(true);
}

void axs15231bPushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                         const uint16_t* data) {
    size_t pixelCount = (size_t)width * height;

    if (y == 0) {
        setColumnRow(x, 0, width, BoardConfig::PANEL_NATIVE_HEIGHT);
    }

    // Write command: 0x2C (new) or 0x3C (continue)
    uint8_t writeCmd = (y == 0) ? 0x2C : 0x3C;

    spi_transaction_ext_t ext = {};
    ext.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR |
                     SPI_TRANS_VARIABLE_DUMMY | SPI_TRANS_MODE_QIO;
    ext.command_bits = 8;
    ext.address_bits = 24;
    ext.base.cmd  = 0x32;
    ext.base.addr = (uint32_t)writeCmd << 8;
    ext.base.tx_buffer = data;
    ext.base.length    = pixelCount * 16;  // bits
    spi_device_polling_transmit(spi_, (spi_transaction_t*)&ext);
}
