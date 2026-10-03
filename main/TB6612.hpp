#pragma once
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "hal/ledc_types.h"
#include "soc/gpio_num.h"

constexpr gpio_num_t AIN1 = GPIO_NUM_18;
constexpr gpio_num_t AIN2 = GPIO_NUM_17;
constexpr gpio_num_t PWMA = GPIO_NUM_16;

constexpr gpio_num_t BIN1 = GPIO_NUM_26;
constexpr gpio_num_t BIN2 = GPIO_NUM_27;
constexpr gpio_num_t PWMB = GPIO_NUM_14;

constexpr gpio_num_t STBY = GPIO_NUM_19;

struct TB6612Pins {
    gpio_num_t AIN1; // used for the directions
    gpio_num_t AIN2;
    gpio_num_t PWMA; // used for speed

    gpio_num_t BIN1;
    gpio_num_t BIN2;
    gpio_num_t PWMB;

    gpio_num_t STBY; // used to see if its in standby or active

    TB6612Pins() = default;
};

class TB6612 {
public:
    TB6612(const TB6612Pins& pins)
        : m_pins(pins) {
        // set pins
        gpio_set_direction(m_pins.AIN1, GPIO_MODE_OUTPUT);
        gpio_set_direction(m_pins.AIN2, GPIO_MODE_OUTPUT);
        gpio_set_direction(m_pins.BIN1, GPIO_MODE_OUTPUT);
        gpio_set_direction(m_pins.BIN2, GPIO_MODE_OUTPUT);
        gpio_set_direction(m_pins.STBY, GPIO_MODE_OUTPUT);

        ledc_timer_config_t timerConfig = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = LEDC_TIMER_8_BIT,
            .timer_num = LEDC_TIMER_0,
            .freq_hz = 20000,
            .clk_cfg = LEDC_AUTO_CLK,
        };

        ledc_timer_config(&timerConfig);

        // Configure PWM channels
        ledc_channel_config_t pwmA = {
            .gpio_num = m_pins.PWMA,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0};

        ledc_channel_config_t pwmB = {
            .gpio_num = PWMB,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = LEDC_CHANNEL_1,
            .timer_sel = LEDC_TIMER_0,
            .duty = 0,
            .hpoint = 0};

        ledc_channel_config(&pwmA);
        ledc_channel_config(&pwmB);
    }

    ~TB6612() {}

private:
    TB6612Pins m_pins;
    static constexpr int MAX_DUTY = 190; // ~75% of 255: keeps the 6V motors safe on a 7-8V pack
};
