#pragma once

#include "driver/i2c_types.h"

class SSD1306 {
public:
    SSD1306(i2c_master_bus_handle_t bus, uint8_t address = 0x3C);
    ~SSD1306();
    SSD1306(const SSD1306&) = delete;
    SSD1306& operator=(const SSD1306&) = delete;

    void Clear();
    void Print(int x, int y, const char* text);    // y in pixels, 8 px per text row
    void Show();                                   // send framebuffer to the screen
    void DrawHeart(int cx, int cy, int size);      // size = half-width in pixels
    void FillRect(int x0, int y0, int x1, int y1); // inclusive corners
    void FillCircle(int cx, int cy, int r);

private:
    void command(uint8_t cmd);
    void setPixel(int x, int y);

    i2c_master_dev_handle_t m_dev{};
    uint8_t m_fb[128 * 8]{}; // 1 bit per pixel, 8 pages of 128 bytes
};
