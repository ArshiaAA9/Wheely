#include "RobotHardware.hpp"

void RobotHardware::Drive() {
    // measurs distance, if too close turn check those direction and move in the empty direction
    // units are in CM
    float distance = m_sonarSensors.MeasureDistanceCM();
    if (distance <= 5.0) {
        // make a 360 turn and go in the direction with highest distance
    }
}

i2c_master_bus_handle_t RobotHardware::createBus(gpio_num_t sda, gpio_num_t scl) {
    i2c_master_bus_config_t cfg{};
    cfg.i2c_port = I2C_NUM_0;
    cfg.sda_io_num = sda;
    cfg.scl_io_num = scl;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus{};
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus));
    return bus;
}
