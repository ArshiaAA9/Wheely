#pragma once
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

constexpr uint8_t REG_CONFIG = 0x1A;
constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
constexpr uint8_t REG_GYRO_ZOUT_H = 0x47;
constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
constexpr uint8_t REG_WHO_AM_I = 0x75;

constexpr float GYRO_LSB_PER_DPS = 65.5f; // for ±500 dps range
constexpr float DEADBAND_DPS = 0.3f;      // ignore rates smaller than this
static const char* TAG = "MPU6050";

class MPU6050 {
public:
    explicit MPU6050(i2c_master_bus_handle_t bus, uint8_t address = 0x68) {
        esp_err_t err = i2c_master_probe(bus, address, 100);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "No device at 0x%02X. Check wiring, power, and AD0.", address);
            ESP_ERROR_CHECK(err);
        }
        i2c_device_config_t dev{};
        dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev.device_address = address;
        dev.scl_speed_hz = 400000;
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev, &m_dev));

        writeReg(REG_PWR_MGMT_1, 0x01); // wake up, use gyro PLL as clock (more stable)
        vTaskDelay(pdMS_TO_TICKS(100));
        writeReg(REG_CONFIG, 0x03);      // low-pass filter ~44 Hz
        writeReg(REG_GYRO_CONFIG, 0x08); // ±500 dps

        uint8_t id = readReg(REG_WHO_AM_I);
        ESP_LOGI(TAG, "WHO_AM_I = 0x%02X (expected 0x68)", id);
    }

    ~MPU6050() {
        if (m_dev) {
            i2c_master_bus_rm_device(m_dev);
        }
    }

    void Calibrate(int samples = 500); // robot must be perfectly still
    void ResetYaw();                   // zero the heading
    void Update();                     // call every ~5-10 ms

    float YawDeg() const { return m_yaw; }

private:
    void writeReg(uint8_t reg, uint8_t val);
    uint8_t readReg(uint8_t reg);
    float readGyroZ(); // deg/s, bias not removed

    i2c_master_dev_handle_t m_dev{};
    float m_bias = 0.0f;
    float m_yaw = 0.0f;
    int64_t m_lastUS = 0; // us stands for microseconds
};
