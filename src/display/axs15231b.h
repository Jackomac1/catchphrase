#pragma once
#include <Arduino.h>

void axs15231bInit();
void axs15231bSetBacklight(bool on);
void axs15231bSetBrightnessPercent(uint8_t percent);
void axs15231bSleep();
void axs15231bWake();
// x, y: physical panel coords (portrait 172×640)
// y==0 starts a new write (0x2C), y!=0 continues (0x3C)
void axs15231bPushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                         const uint16_t* data);
