#pragma once
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "soc/gpio_num.h"

class MPU6050 {
public:
    MPU6050(i2c_master_bus_handle_t bus, uint8_t address = 0x68) {}

    void update();

    float yawDeg() const { return m_yaw; }

private:
    i2c_master_dev_handle_t m_dev{};
    float m_yaw = 0.0f;
    int64_t m_lastUs = 0;
};
