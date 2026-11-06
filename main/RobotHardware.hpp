#pragma once
#include "HC-SRO4.hpp"
#include "MPU6050.hpp"
#include "TB6612.hpp"
#include "soc/gpio_num.h"

class RobotHardware {
    struct Pins {
        TB6612::Pins TB6612Pins;
        HC_SRO4::Pins HC_SRO4Pins;
        gpio_num_t SDA;
        gpio_num_t SCL;
    };

public:
    RobotHardware(RobotHardware::Pins& pins)
        : m_i2cBus(createBus(pins.SDA, pins.SCL))
        , m_motors(pins.TB6612Pins)
        , m_sonar(pins.HC_SRO4Pins)
        , m_gyroscope(m_i2cBus) {

        m_motors.SetSpeedToMax();
    }

    ~RobotHardware();

    void Drive();

private:
    static i2c_master_bus_handle_t createBus(gpio_num_t sda, gpio_num_t scl);

    i2c_master_bus_handle_t m_i2cBus; // must come BEFORE its used
    TB6612 m_motors;
    HC_SRO4 m_sonar;
    MPU6050 m_gyroscope;
};
