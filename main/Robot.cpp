#include "Robot.hpp"

#include <algorithm>
#include <cmath>

#include "esp_timer.h"

float Robot::scan360(std::atomic<bool>& isRunning) {
    constexpr int BIN_DEG = 10;
    constexpr int BINS = 360 / BIN_DEG;     // 36
    constexpr float NO_ECHO_CM = 200.0f;    // no echo = treat as "far"
    constexpr int64_t PING_GAP_US = 60'000; // HC-SR04 needs ~60 ms between pings
                                            //
    float minCm[BINS];
    bool seen[BINS] = {};

    for (auto& v : minCm) v = 1e9f;

    Gyroscope.ResetYaw();
    Motors.TurnLeft();

    int64_t lastPing = 0;
    const int64_t scanStart = esp_timer_get_time();
    while (Gyroscope.YawDeg() < 360.0f && isRunning && esp_timer_get_time() - scanStart < 20'000'000) {
        Gyroscope.Update();

        int64_t now = esp_timer_get_time();
        if (now - lastPing >= PING_GAP_US) {
            lastPing = now;

            float yaw = Gyroscope.YawDeg(); // sample the angle at the ping
            float d = Sonar.MeasureDistanceCM();
            if (d < 0) d = NO_ECHO_CM;
            d = std::min(d, NO_ECHO_CM);

            int bin = static_cast<int>(yaw) / BIN_DEG;
            if (bin >= 0 && bin < BINS) {
                minCm[bin] = std::min(minCm[bin], d); // keep the closest object per bin
                seen[bin] = true;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    Motors.Brake();
    // Score each bin with its two neighbours so one lucky reading can't win
    int best = 0;
    float bestScore = -1;
    for (int i = 0; i < BINS; i++) {
        float s = 0;
        for (int k = -1; k <= 1; k++) {
            int j = (i + k + BINS) % BINS;
            s += seen[j] ? minCm[j] : 0.0f; // unknown counts as blocked
        }
        if (s > bestScore) {
            bestScore = s;
            best = i;
        }
    }
    return best * BIN_DEG + BIN_DEG / 2.0f; // centre of the best bin
}

void Robot::Drive(std::atomic<bool>& isRunning) {
    // thsi function is ran every tick of the robot
    // measurs distance, if too close turn check those direction and move in the
    // empty direction units are in CM
    Gyroscope.Update();
    float distance = Sonar.MeasureDistanceCM();

    if (distance > 0 && distance <= 20.0f) {
        // make a 360 turn and go in the direction with highest distance
        float best = scan360(isRunning);
        if (!isRunning) return;

        float current = Gyroscope.YawDeg();
        float delta = wrap180(best - std::fmod(current, 360.0f));
        turnToHeading(current + delta, isRunning);
    }
    Motors.Forward();
}

i2c_master_bus_handle_t Robot::createBus(gpio_num_t sda, gpio_num_t scl) {
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

float Robot::wrap180(float a) {
    while (a > 180.0f) a -= 360.0f;
    while (a < -180.0f) a += 360.0f;
    return a;
}

bool Robot::turnToHeading(float targetDeg, std::atomic<bool>& isRunning) {
    constexpr float HEADING_TOLERANCE_DEG = 3.0f; // "close enough"
    constexpr float SLOW_ZONE_DEG = 30.0f;        // start slowing within this error
    constexpr int64_t TURN_TIMEOUT_US = 8'000'000;

    const int64_t start = esp_timer_get_time();

    while (esp_timer_get_time() - start < TURN_TIMEOUT_US && isRunning) {
        Gyroscope.Update();

        float err = targetDeg - Gyroscope.YawDeg(); // + means turn CCW

        if (std::fabs(err) < HEADING_TOLERANCE_DEG) {
            Motors.Brake();
            return true;
        }

        bool fast = std::fabs(err) > SLOW_ZONE_DEG;
        if (err > 0) Motors.TurnLeft(); // CCW, yaw increases
        else
            Motors.TurnRight(); // CW, yaw decreases

        vTaskDelay(pdMS_TO_TICKS(5));
    }

    Motors.Brake();
    return false;
}
