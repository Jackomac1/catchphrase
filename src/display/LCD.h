#pragma once
#include <LovyanGFX.hpp>
#include "board/BoardConfig.h"

class LGFX : public lgfx::LGFX_Device {
public:
    LGFX() {
        {
            auto cfg = _bus.config();
            cfg.spi_host    = SPI3_HOST;
            cfg.freq_write  = 80000000;
            cfg.pin_sclk    = BoardConfig::PIN_LCD_SCLK;
            cfg.pin_d0      = BoardConfig::PIN_LCD_DATA0;
            cfg.pin_d1      = BoardConfig::PIN_LCD_DATA1;
            cfg.pin_d2      = BoardConfig::PIN_LCD_DATA2;
            cfg.pin_d3      = BoardConfig::PIN_LCD_DATA3;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs           = BoardConfig::PIN_LCD_CS;
            cfg.pin_rst          = BoardConfig::PIN_LCD_RST;
            cfg.pin_busy         = -1;
            cfg.panel_width      = 172;
            cfg.panel_height     = 640;
            cfg.memory_width     = 172;
            cfg.memory_height    = 640;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.readable         = false;
            cfg.invert           = false;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl      = BoardConfig::PIN_LCD_BL;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 0;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        {
            auto cfg = _touch.config();
            cfg.i2c_addr  = 0x3B;
            cfg.i2c_port  = I2C_NUM_0;
            cfg.pin_sda   = BoardConfig::PIN_TOUCH_SDA;
            cfg.pin_scl   = BoardConfig::PIN_TOUCH_SCL;
            cfg.pin_int   = -1;
            cfg.pin_rst   = -1;
            cfg.x_min     = 0;
            cfg.x_max     = 639;
            cfg.y_min     = 0;
            cfg.y_max     = 171;
            cfg.bus_shared = false;
            _touch.config(cfg);
            _panel.setTouch(&_touch);
        }
        setPanel(&_panel);
    }

private:
    lgfx::Panel_AXS15231B  _panel;
    lgfx::Bus_QSPI         _bus;
    lgfx::Light_PWM        _light;
    lgfx::Touch_AXS15231B  _touch;
};
