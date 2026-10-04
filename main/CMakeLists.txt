idf_component_register(
    SRCS
        "wheely.cpp"
        "TB6612.cpp"
        "WifiController.cpp"
        "TCPServer.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_ledc
        esp_wifi
        nvs_flash
)
