#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <string_view>

#include "TB6612.hpp"
#include "lwip/sockets.h"

class TCPServer {
public:
    explicit TCPServer(TB6612& motors)
        : m_motors(motors) {}

    ~TCPServer() { Stop(); }

    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;

    /// Binds, listens and starts the server task. Safe before Wi-Fi has an IP (binds to any address).
    esp_err_t StartListening();

    /// Stops the task, closes sockets and brakes the motors. Idempotent.
    /// Never call it from inside the server task itself.
    void Stop();

private:
    static void taskEntry(void* arg); // static: only exists to satisfy xTaskCreate
    void tcpServerTask();
    void handleClient();
    static std::optional<TB6612::MotionState> parseCommand(std::string_view command);
    void handleCommand(std::string_view);

    static constexpr std::string_view m_TAG = "TCP_SERVER";
    static constexpr uint16_t m_PORT = 3333;
    static constexpr int m_ACCEPT_TIMEOUT_S = 1; // lets the task notice stop() requests

    int m_socket{-1};
    int m_client{-1}; // client socket
    std::atomic<bool> m_running{false};
    bool m_taskStarted{false};
    std::atomic<bool> m_taskRunning{false}; // "the task is still alive" (cleared by the task itself)

    TB6612& m_motors;
};
