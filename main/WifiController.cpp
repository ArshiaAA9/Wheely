#include "WifiController.hpp"

#include <cstring>

#include "esp_log.h"
#include "esp_wifi.h"

WifiController::WifiController() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiController::eventHandler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiController::eventHandler, this));

    wifi_config_t wifiConfig{};

    strlcpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), WIFI_SSID, sizeof(wifiConfig.sta.ssid));
    strlcpy(reinterpret_cast<char*>(wifiConfig.sta.password), WIFI_PASSWORD, sizeof(wifiConfig.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
}

WifiController::~WifiController() {
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler);
    esp_wifi_stop();
    esp_wifi_deinit();
}

void WifiController::eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    auto* controller = static_cast<WifiController*>(arg);

    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_START) {

        ESP_LOGI(TAG, "WiFi started");

        esp_wifi_connect();
    }

    else if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {

        if (controller->m_retryCount < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            controller->m_retryCount++;

            ESP_LOGI(TAG, "Retrying connection...");
        } else {
            ESP_LOGE(TAG, "Failed to connect to WiFi");
        }
    }

    else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {

        auto* event = static_cast<ip_event_got_ip_t*>(eventData);

        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));

        controller->m_retryCount = 0;
    }
}
