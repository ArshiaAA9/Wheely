#pragma once
#include <atomic>

#include "HC-SRO4.hpp"
#include "MPU6050.hpp"
#include "TB6612.hpp"
#include "esp_log.h"
#include "soc/gpio_num.h"

class Robot {
public:
    struct Pins {
        TB6612::Pins TB6612Pins;
        HC_SRO4::Pins HC_SRO4Pins;
        gpio_num_t SDA;
        gpio_num_t SCL;
    };

    Robot(const Robot::Pins& pins)
        : I2cBus(createBus(pins.SDA, pins.SCL))
        , Motors(pins.TB6612Pins)
        , Sonar(pins.HC_SRO4Pins)
        , Gyroscope(I2cBus) {

        Motors.SetSpeedToMax();
    }

    ~Robot() {
        ESP_LOGI("Robot", "Destroying object");
        Motors.Brake();
    }

    Robot(const Robot&) = delete;
    Robot& operator=(const Robot&) = delete;

    float scan360(std::atomic<bool>& isRunning);
    void Drive(std::atomic<bool>& isRunning);

    i2c_master_bus_handle_t I2cBus; // must come BEFORE its used
    TB6612 Motors;
    HC_SRO4 Sonar;
    MPU6050 Gyroscope;

private:
    static i2c_master_bus_handle_t createBus(gpio_num_t sda, gpio_num_t scl);
    static float wrap180(float a);
    bool turnToHeading(float targetDeg, std::atomic<bool>& isRunning);
};
