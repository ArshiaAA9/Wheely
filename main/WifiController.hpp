#pragma once
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"

class WifiController {
    WifiController() {
        esp_netif_init();
        esp_event_loop_create_default();

        esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = wifi_init_config_t();
        esp_wifi_init(&cfg);
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        esp_wifi_connect();
    }
};
