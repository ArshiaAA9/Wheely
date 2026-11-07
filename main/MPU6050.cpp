#include "MPU6050.hpp"

#include <cmath>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

void MPU6050::Calibrate(int samples) {
    ESP_LOGI(TAG, "Calibrating, keep the robot still...");
    float sum = 0.0f;
    for (int i = 0; i < samples; i++) {
        sum += readGyroZ();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    m_bias = sum / samples;
    ESP_LOGI(TAG, "Gyro Z bias = %.3f deg/s", m_bias);
    ResetYaw();
}

void MPU6050::ResetYaw() {
    m_yaw = 0.0f;
    m_lastUS = esp_timer_get_time();
}

void MPU6050::Update() {
    int64_t now = esp_timer_get_time();
    float dt = (now - m_lastUS) / 1e6f;
    m_lastUS = now;

    float rate = readGyroZ() - m_bias;
    if (std::fabs(rate) > DEADBAND_DPS) {
        m_yaw += rate * dt; // deg/s × s = degrees
    }
}

void MPU6050::ReadAccel(float& ax, float& ay) {
    uint8_t reg = 0x3B; // ACCEL_XOUT_H
    uint8_t b[4];       // X high/low, Y high/low
    ESP_ERROR_CHECK(i2c_master_transmit_receive(m_dev, &reg, 1, b, 4, 100));
    ax = static_cast<int16_t>((b[0] << 8) | b[1]) / 16384.0f; // default ±2 g range
    ay = static_cast<int16_t>((b[2] << 8) | b[3]) / 16384.0f;
    ax = -ax; // board is upside down: X is mirrored (flip if it moves the wrong way)
}

void MPU6050::writeReg(uint8_t reg, uint8_t val) {
    uint8_t data[2] = {reg, val};
    ESP_ERROR_CHECK(i2c_master_transmit(m_dev, data, 2, 100));
}

uint8_t MPU6050::readReg(uint8_t reg) {
    uint8_t val = 0;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(m_dev, &reg, 1, &val, 1, 100));
    return val;
}

float MPU6050::readGyroZ() {
    uint8_t reg = REG_GYRO_ZOUT_H;
    uint8_t buf[2];
    ESP_ERROR_CHECK(i2c_master_transmit_receive(m_dev, &reg, 1, buf, 2, 100));
    int16_t raw = static_cast<int16_t>((buf[0] << 8) | buf[1]);
    return raw / GYRO_LSB_PER_DPS * -1; // multiply by -1 because the mpu is mounted upside down
}
