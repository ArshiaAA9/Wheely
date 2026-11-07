#include <cstdio>

#include "Robot.hpp"
#include "TB6612.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "soc/gpio_num.h"

constexpr gpio_num_t AIN1 = GPIO_NUM_18;
constexpr gpio_num_t AIN2 = GPIO_NUM_17;
constexpr gpio_num_t PWMA = GPIO_NUM_16;

constexpr gpio_num_t BIN1 = GPIO_NUM_26;
constexpr gpio_num_t BIN2 = GPIO_NUM_27;
constexpr gpio_num_t PWMB = GPIO_NUM_14;
constexpr gpio_num_t STBY = GPIO_NUM_19;

constexpr gpio_num_t TRIGGER = GPIO_NUM_25;
constexpr gpio_num_t ECHO = GPIO_NUM_33;

constexpr gpio_num_t SDA = GPIO_NUM_21;
constexpr gpio_num_t SCL = GPIO_NUM_22;

constexpr gpio_num_t START_BTN = GPIO_NUM_35;

static std::atomic<bool> isRunning{false};

static void buttonTask(void*) {
    bool lastLevel = false;
    int64_t lastChangeUs = 0;

    ESP_LOGI("Main", "Waiting for button input");
    while (true) {
        bool level = gpio_get_level(START_BTN) == 1;
        int64_t now = esp_timer_get_time();

        if (level != lastLevel && now - lastChangeUs > 50'000) {
            lastChangeUs = now;
            lastLevel = level;
            if (level) { // pressed
                isRunning = !isRunning;
                if (isRunning) {
                    ESP_LOGI("Main", "START");
                } else {
                    ESP_LOGI("Main", "STOP");
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// in app_main, after configuring the pin:

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(gpio_set_direction(START_BTN, GPIO_MODE_INPUT));

    // static WifiController wifiController{};

    TB6612::Pins motorsPins{};
    motorsPins.AIN1 = AIN1;
    motorsPins.AIN2 = AIN2;
    motorsPins.PWMA = PWMA;
    motorsPins.BIN1 = BIN1;
    motorsPins.BIN2 = BIN2;
    motorsPins.PWMB = PWMB;
    motorsPins.STBY = STBY;

    Robot::Pins pins{motorsPins, {TRIGGER, ECHO}, SDA, SCL};

    // wait for start button
    xTaskCreate(buttonTask, "button", 2048, nullptr, 5, nullptr);

    while (true) {
        if (!isRunning) {
            vTaskDelay(pdMS_TO_TICKS(20)); // idle, waiting for a press
            continue;
        }

        {
            Robot robot{pins};

            vTaskDelay(pdMS_TO_TICKS(1000)); // let go of the board
            robot.Gyroscope.Calibrate();

            while (isRunning) {
                robot.Drive(isRunning);
                vTaskDelay(pdMS_TO_TICKS(5));
            }
        } // robot destroyed here
        ESP_LOGI("Main", "Loop exited");
    }
}
