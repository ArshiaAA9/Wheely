#include "TCPServer.hpp"

#include <cstring>

#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

esp_err_t TCPServer::StartListening() {

    if (m_taskRunning) {
        return ESP_ERR_INVALID_STATE;
    }

    if (m_running) {
        return ESP_ERR_INVALID_STATE;
    }

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (m_socket < 0) {
        ESP_LOGE(m_TAG.data(), "Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }

    int opt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // accept() will time out periodically so the task can check m_running.
    timeval timeout{};
    timeout.tv_sec = m_ACCEPT_TIMEOUT_S;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_PORT);
    if (bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        ESP_LOGE(m_TAG.data(), "bind failed: errno %d", errno);
        ::close(m_socket);
        m_socket = -1;
        return ESP_FAIL;
    }
    if (listen(m_socket, 1) != 0) {
        ESP_LOGE(m_TAG.data(), "listen failed: errno %d", errno);
        ::close(m_socket);
        m_socket = -1;
        return ESP_FAIL;
    }
    // Both flags are set BEFORE the task exists, so Stop() can never miss a task that is about to start.
    m_running = true;
    m_taskRunning = true;
    ESP_LOGI(m_TAG.data(), "Starting the server");

    if (xTaskCreate(&TCPServer::taskEntry, "tcp_server", 4096, this, 5, nullptr) != pdPASS) {
        ESP_LOGE(m_TAG.data(), "Failed to create task");
        m_running = false;
        ::close(m_socket);
        m_socket = -1;
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void TCPServer::taskEntry(void* arg) { static_cast<TCPServer*>(arg)->tcpServerTask(); }

void TCPServer::Stop() {
    m_running = false;

    // Wait for the task to clear m_taskRunning (it wakes at least every m_ACCEPT_TIMEOUT_S).
    const TickType_t maxWait = pdMS_TO_TICKS((m_ACCEPT_TIMEOUT_S + 2) * 1000);
    const TickType_t start = xTaskGetTickCount();
    while (m_taskRunning && (xTaskGetTickCount() - start) < maxWait) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (m_taskRunning) {
        ESP_LOGE(m_TAG.data(), "Server task did not exit in time");
    }

    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }

    m_motors.Brake();
}

void TCPServer::tcpServerTask() {
    while (m_running) {
        sockaddr_in sourceAddr{};
        socklen_t addrLen = sizeof(sourceAddr);
        const int client = accept(m_socket, reinterpret_cast<sockaddr*>(&sourceAddr), &addrLen);

        if (client < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // timeout: loop and re-check m_running
            }
            ESP_LOGE(m_TAG.data(), "accept failed: errno %d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        m_client = client;

        char addrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sourceAddr.sin_addr, addrStr, sizeof(addrStr));
        ESP_LOGI(m_TAG.data(), "Client connected from %s", addrStr);

        handleClient(); // one client at a time, fine for a single controller

        shutdown(m_client, SHUT_RDWR);
        close(m_client);
        ESP_LOGI(m_TAG.data(), "Client disconnected");
    }
    m_taskRunning = false;
    vTaskDelete(nullptr);
}

void TCPServer::handleClient() {

    // Reads time out too, so a silent client can't block stop().
    timeval timeout{};
    timeout.tv_sec = m_ACCEPT_TIMEOUT_S;
    setsockopt(m_client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    int on = 1;
    setsockopt(m_client, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

    char rx[128];
    size_t used = 0; // bytes of an incomplete line carried over between recv() calls
    while (m_running) {
        const int n = recv(m_client, rx, sizeof(rx), 0);
        if (n > 0) {
            used += static_cast<size_t>(n);

            // Handle every complete "\n"-terminated line in the buffer.
            size_t start = 0;
            for (size_t i = 0; i < used; ++i) {
                if (rx[i] != '\n') continue;
                size_t end = i;
                if (end > start && rx[end - 1] == '\r') --end; // tolerate CRLF
                handleCommand(std::string_view(rx + start, end - start));
                start = i + 1;
            }

            // Keep the unfinished tail for the next recv().
            std::memmove(rx, rx + start, used - start);
            used -= start;

            if (used == sizeof(rx)) { // a "line" this long is garbage; avoid a zero-size recv
                ESP_LOGW(m_TAG.data(), "Input line too long, discarding");
                used = 0;
            }
        } else if (n == 0) {
            break;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            ESP_LOGE(m_TAG.data(), "recv failed: errno %d", errno);
            break;
        }
    }
    m_motors.Brake();
}

std::optional<TB6612::MotionState> TCPServer::parseCommand(std::string_view command) {
    using State = TB6612::MotionState;

    if (command == "F") return State::Forward;
    if (command == "R") return State::Reverse;
    if (command == "TR") return State::TurnRight;
    if (command == "TL") return State::TurnLeft;
    if (command == "BR") return State::Brake;
    if (command == "C") return State::Coast;
    return std::nullopt;
}

// one received line -> motor action
void TCPServer::handleCommand(std::string_view command) {
    // parse command and turn it into an enum
    const auto parsed = parseCommand(command);
    if (!parsed) {
        ESP_LOGW(m_TAG.data(), "Unknown command (%u bytes), braking", static_cast<unsigned>(command.size()));
    }
    // Garbage while moving: stopping is the safe choice.
    const TB6612::MotionState code = parsed.value_or(TB6612::MotionState::Brake);

    // use the enum in switch
    switch (code) {
        case TB6612::MotionState::Forward:
            m_motors.Forward();
            break;
        case TB6612::MotionState::Reverse:
            m_motors.Reverse();
            break;
        case TB6612::MotionState::TurnRight:
            m_motors.TurnRight();
            break;
        case TB6612::MotionState::TurnLeft:
            m_motors.TurnLeft();
            break;
        case TB6612::MotionState::Brake:
            m_motors.Brake();
            break;
        case TB6612::MotionState::Coast:
            m_motors.Coast();
            break;
    }
}
