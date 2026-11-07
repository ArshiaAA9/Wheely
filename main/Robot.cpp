#include "Robot.hpp"

#include <algorithm>
#include <cmath>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
            m_lastDistance = d;
            if (d < 0) d = NO_ECHO_CM;
            d = std::min(d, NO_ECHO_CM);

            int bin = static_cast<int>(yaw) / BIN_DEG;
            if (bin >= 0 && bin < BINS) {
                minCm[bin] = std::min(minCm[bin], d); // keep the closest object per bin
                seen[bin] = true;
            }
        }
        UpdateScreen("Scanning"); // refreshes at most every 500 ms
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

    constexpr float STOP_CM = 20.0f;
    constexpr int64_t PING_GAP_US = 60'000;

    Gyroscope.Update(); // keep yaw current every call

    const int64_t now = esp_timer_get_time();
    if (now - m_lastPingUs < PING_GAP_US) {
        return; // too soon to ping, keep doing what we're doing
    }
    m_lastPingUs = now;

    float distance = Sonar.MeasureDistanceCM();
    m_lastDistance = distance;
    UpdateScreen("Driving");

    bool tooClose = (distance > 0 && distance <= STOP_CM);

    // Require 2 close readings in a row, so one glitch doesn't trigger a scan
    m_closeCount = tooClose ? m_closeCount + 1 : 0;

    if (m_closeCount >= 2) {
        m_closeCount = 0;
        m_moving = false;
        Motors.Brake();

        float best = scan360(isRunning);
        if (!isRunning) return;

        float current = Gyroscope.YawDeg();
        float delta = wrap180(best - std::fmod(current, 360.0f));
        turnToHeading(current + delta, isRunning);
        return;
    }

    if (!m_moving) { // only send the command once
        Motors.Forward();
        m_moving = true;
    }
}

void Robot::UpdateScreen(const char* mode) {
    int64_t now = esp_timer_get_time();
    if (now - m_lastScreenUs < 500'000) return;
    m_lastScreenUs = now;

    char line[24];
    Screen.Clear();
    snprintf(line, sizeof(line), "Dist: %.0f cm", m_lastDistance);
    Screen.Print(0, 0, line);
    snprintf(line, sizeof(line), "Yaw:  %.0f deg", Gyroscope.YawDeg());
    Screen.Print(0, 16, line);
    snprintf(line, sizeof(line), "Mode: %s", mode);
    Screen.Print(0, 32, line);
    Screen.Show();
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
    constexpr int64_t TURN_TIMEOUT_US = 8'000'000;

    const int64_t start = esp_timer_get_time();

    while (esp_timer_get_time() - start < TURN_TIMEOUT_US && isRunning) {
        Gyroscope.Update();

        float err = targetDeg - Gyroscope.YawDeg(); // + means turn CCW

        if (std::fabs(err) < HEADING_TOLERANCE_DEG) {
            Motors.Brake();
            return true;
        }

        if (err > 0) Motors.TurnLeft(); // CCW, yaw increases
        else
            Motors.TurnRight(); // CW, yaw decreases
                                //
        if (std::fabs(err) > 15.0f) UpdateScreen("Turning");
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    Motors.Brake();
    return false;
}
