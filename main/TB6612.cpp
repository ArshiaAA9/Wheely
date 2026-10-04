#include "TB6612.hpp"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "hal/ledc_types.h"

void TB6612::init() {
    // set pins
    gpio_set_direction(m_pins.AIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(m_pins.AIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(m_pins.BIN1, GPIO_MODE_OUTPUT);
    gpio_set_direction(m_pins.BIN2, GPIO_MODE_OUTPUT);
    gpio_set_direction(m_pins.STBY, GPIO_MODE_OUTPUT);

    ledc_timer_config_t timerConfig{};

    timerConfig.speed_mode = LEDC_LOW_SPEED_MODE;
    timerConfig.duty_resolution = LEDC_TIMER_8_BIT;
    timerConfig.timer_num = LEDC_TIMER_0;
    timerConfig.freq_hz = 20000;
    timerConfig.clk_cfg = LEDC_AUTO_CLK;

    ledc_timer_config(&timerConfig);

    // PWM channels configs
    ledc_channel_config_t pwmA = {};
    pwmA.gpio_num = m_pins.PWMA;
    pwmA.speed_mode = LEDC_LOW_SPEED_MODE;
    pwmA.channel = LEDC_CHANNEL_0; // using channel 0
    pwmA.timer_sel = LEDC_TIMER_0;
    pwmA.duty = 0;
    pwmA.hpoint = 0;

    ledc_channel_config_t pwmB = {};
    pwmB.gpio_num = m_pins.PWMB;
    pwmB.speed_mode = LEDC_LOW_SPEED_MODE;
    pwmB.channel = LEDC_CHANNEL_1; // using channel 1
    pwmB.timer_sel = LEDC_TIMER_0;
    pwmB.duty = 0;
    pwmB.hpoint = 0;

    ledc_channel_config(&pwmA);
    ledc_channel_config(&pwmB);

    // 1 high, 0 low
    gpio_set_level(m_pins.STBY, 1);
}

void TB6612::setSpeedToMax() {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, m_MAX_DUTY);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, m_MAX_DUTY);

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void TB6612::Forward() {
    m_state = MotionState::Forward;
    gpio_set_level(m_pins.AIN1, 1);
    gpio_set_level(m_pins.AIN2, 0);

    gpio_set_level(m_pins.BIN1, 1);
    gpio_set_level(m_pins.BIN2, 0);

    ESP_LOGI("MOTOR", "Driving forward");
}

void TB6612::Reverse() {
    m_state = MotionState::Reverse;
    gpio_set_level(m_pins.AIN1, 0);
    gpio_set_level(m_pins.AIN2, 1);

    gpio_set_level(m_pins.BIN1, 0);
    gpio_set_level(m_pins.BIN2, 1);
}

void TB6612::TurnRight() {
    m_state = MotionState::TurnRight;
    // right motor
    gpio_set_level(m_pins.AIN1, 0);
    gpio_set_level(m_pins.AIN2, 1);

    // left motor
    gpio_set_level(m_pins.BIN1, 1);
    gpio_set_level(m_pins.BIN2, 0);
}

void TB6612::TurnLeft() {
    m_state = MotionState::TurnRight;
    // right motor
    gpio_set_level(m_pins.AIN1, 1);
    gpio_set_level(m_pins.AIN2, 0);

    // left motor
    gpio_set_level(m_pins.BIN1, 0);
    gpio_set_level(m_pins.BIN2, 1);
}
