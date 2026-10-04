#pragma once
#include "esp_event.h"
#include "esp_wifi.h"

#define WIFI_SSID CONFIG_ESP_WIFI_SSID
#define WIFI_PASSWORD CONFIG_ESP_WIFI_PASSWORD
#define WIFI_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

class WifiController {
public:
    WifiController();
    WifiController(const WifiController&) = delete;
    WifiController& operator=(const WifiController&) = delete;
    ~WifiController();

private:
    static void eventHandler(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    static constexpr char TAG[] = "WIFI";
    int m_retryCount = 0;
};
