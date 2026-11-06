#include <cstdio>

#include "DriveController.hpp"
#include "HC-SRO4.hpp"
#include "TB6612.hpp"
#include "TCPServer.hpp"
#include "WebServer.hpp"
#include "WifiController.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
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

extern "C" void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    static WifiController wifiController{};

    TB6612::Pins motorsPins{};
    motorsPins.AIN1 = AIN1;
    motorsPins.AIN2 = AIN2;
    motorsPins.PWMA = PWMA;
    motorsPins.BIN1 = BIN1;
    motorsPins.BIN2 = BIN2;
    motorsPins.PWMB = PWMB;
    motorsPins.STBY = STBY;

    DriveController driver{};

    // static TCPServer server{motors};
    // server.StartListening();
    //
    // static WebServer web{motors};
    // ESP_ERROR_CHECK(web.Start());

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
