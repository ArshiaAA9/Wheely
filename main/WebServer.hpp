#pragma once

#include <atomic>
#include <cstdint>

#include "TB6612.hpp"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_timer.h"

/// Serves the D-pad page on port 80 and takes commands over a WebSocket at /ws.
/// Brakes automatically if no command arrives for a moment (finger lifted, Wi-Fi lost).
class WebServer {
public:
    explicit WebServer(TB6612& motors)
        : m_motors(motors) {}

    ~WebServer() { Stop(); }

    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    esp_err_t Start();
    void Stop();

private:
    static esp_err_t indexHandler(httpd_req_t* req);
    static esp_err_t wsHandler(httpd_req_t* req);
    static void watchdogCb(void* arg);
    void handleCommand(const char* cmd);

    static constexpr int64_t m_WATCHDOG_US = 600'000;

    TB6612& m_motors;
    httpd_handle_t m_server{nullptr};
    esp_timer_handle_t m_timer{nullptr};
    std::atomic<bool> m_moving{false};
    std::atomic<int64_t> m_lastCmdUs{0};
};
