#include "HC-SRO4.hpp"

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "hal/gpio_types.h"

float HC_SRO4::MeasureDistanceCM() {
    // Send a 10 µs trigger pulse
    gpio_set_level(m_pins.TRIG, 0);
    esp_rom_delay_us(2);
    gpio_set_level(m_pins.TRIG, 1);
    esp_rom_delay_us(10);
    gpio_set_level(m_pins.TRIG, 0);

    // Wait for Echo to go HIGH
    int64_t start_wait = esp_timer_get_time();
    while (gpio_get_level(m_pins.ECHO) == 0) {
        if (esp_timer_get_time() - start_wait > m_timeOut) {
            return -1;
        }
    }

    // Measure how long Echo stays HIGH
    int64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(m_pins.ECHO) == 1) {
        if (esp_timer_get_time() - echo_start > m_timeOut) {
            return -1;
        }
    }
    int64_t echo_end = esp_timer_get_time();

    int64_t duration = echo_end - echo_start;
    return duration * 0.0343f / 2.0f;
}
