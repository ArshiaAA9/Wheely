idf_component_register(
    SRCS
        "wheely.cpp"
        "TB6612.cpp"
        "HC-SRO4.cpp"
        "Robot.cpp"
        "MPU6050.cpp"
        "SSD1306.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_ledc
        esp_wifi
        nvs_flash
        esp_http_server
        esp_timer
        esp_driver_i2c
)
