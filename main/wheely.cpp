#include <cstdio>

#include "TB6612.hpp"
#include "freertos/FreeRTOS.h"

constexpr gpio_num_t AIN1 = GPIO_NUM_18;
constexpr gpio_num_t AIN2 = GPIO_NUM_17;
constexpr gpio_num_t PWMA = GPIO_NUM_16;

constexpr gpio_num_t BIN1 = GPIO_NUM_26;
constexpr gpio_num_t BIN2 = GPIO_NUM_27;
constexpr gpio_num_t PWMB = GPIO_NUM_14;
constexpr gpio_num_t STBY = GPIO_NUM_19;

extern "C" void app_main(void) {
    TB6612Pins motorsPins{};
    motorsPins.AIN1 = AIN1;
    motorsPins.AIN2 = AIN2;
    motorsPins.PWMA = PWMA;
    motorsPins.BIN1 = BIN1;
    motorsPins.BIN2 = BIN2;
    motorsPins.PWMB = PWMB;
    motorsPins.STBY = STBY;

    TB6612 motors{motorsPins};
    motors.TurnRight();
    motors.setSpeedToMax();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
