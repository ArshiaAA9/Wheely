#include "SSD1306.hpp"

#include <cstring>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "font8x8_basic.h"

static const char* TAG = "SSD1306";

SSD1306::SSD1306(i2c_master_bus_handle_t bus, uint8_t address) {
    if (i2c_master_probe(bus, address, 100) != ESP_OK) {
        ESP_LOGE(TAG, "No display at 0x%02X (try 0x3D)", address);
        abort();
    }

    i2c_device_config_t dev{};
    dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev.device_address = address;
    dev.scl_speed_hz = 400000;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev, &m_dev));

    const uint8_t init[] = {
        0xAE,       // display off
        0xD5, 0x80, // clock divide
        0xA8, 0x3F, // 64 rows
        0xD3, 0x00, // no vertical offset
        0x40,       // start line 0
        0x8D, 0x14, // enable charge pump
        0x20, 0x02, // page addressing mode
        0xA0,       // dont flip horizontally
        0xC0,       // flip vertically
        0xDA, 0x12, // COM pins config
        0x81, 0xCF, // contrast
        0xD9, 0xF1, // pre-charge
        0xDB, 0x40, // VCOM level
        0xA4,       // show RAM contents
        0xA6,       // normal (not inverted)
        0xAF,       // display on
    };
    for (uint8_t c : init) command(c);

    Clear();
    Show();
}

SSD1306::~SSD1306() {
    if (m_dev) {
        command(0xAE); // display off
        i2c_master_bus_rm_device(m_dev);
    }
}

void SSD1306::command(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd}; // 0x00 = "this is a command"
    ESP_ERROR_CHECK(i2c_master_transmit(m_dev, data, 2, 100));
}

void SSD1306::Clear() { memset(m_fb, 0, sizeof(m_fb)); }

void SSD1306::setPixel(int x, int y) {
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    m_fb[(y / 8) * 128 + x] |= (1 << (y % 8));
}

void SSD1306::Print(int x, int y, const char* text) {
    for (; *text; text++, x += 8) {
        unsigned char ch = static_cast<unsigned char>(*text);
        if (ch > 127) ch = '?';
        for (int row = 0; row < 8; row++) {
            uint8_t bits = font8x8_basic[ch][row];
            for (int col = 0; col < 8; col++) {
                if (bits & (1 << col)) setPixel(x + col, y + row); // bit 0 = leftmost
            }
        }
    }
}

void SSD1306::Show() {
    for (int page = 0; page < 8; page++) {
        command(0xB0 + page); // select page
        command(0x00);        // column low nibble = 0
        command(0x10);        // column high nibble = 0

        uint8_t buf[1 + 128];
        buf[0] = 0x40; // 0x40 = "this is display data"
        memcpy(buf + 1, &m_fb[page * 128], 128);
        ESP_ERROR_CHECK(i2c_master_transmit(m_dev, buf, sizeof(buf), 100));
    }
}

void SSD1306::DrawHeart(int cx, int cy, int size) {
    for (int py = 0; py < 64; py++) {
        for (int px = 0; px < 128; px++) {
            // Map the pixel to heart-equation space, y pointing up
            float x = (px - cx) / static_cast<float>(size) * 1.3f;
            float y = (cy - py) / static_cast<float>(size) * 1.3f;

            // (x² + y² - 1)³ - x²y³ <= 0  is the inside of a heart
            float a = x * x + y * y - 1.0f;
            if (a * a * a - x * x * y * y * y <= 0.0f) setPixel(px, py);
        }
    }
}

void SSD1306::FillRect(int x0, int y0, int x1, int y1) {
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) setPixel(x, y);
}

void SSD1306::FillCircle(int cx, int cy, int r) {
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r) setPixel(cx + x, cy + y);
}
