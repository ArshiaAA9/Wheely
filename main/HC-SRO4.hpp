#pragma once

#include "driver/gpio.h"
#include "soc/gpio_num.h"

class HC_SRO4 {
public:
    struct Pins {
        gpio_num_t TRIG;
        gpio_num_t ECHO;
    };

    HC_SRO4(HC_SRO4::Pins pins)
        : m_pins(pins) {
        gpio_set_direction(m_pins.TRIG, GPIO_MODE_OUTPUT);
        gpio_set_direction(m_pins.ECHO, GPIO_MODE_INPUT);

        gpio_set_level(m_pins.TRIG, 0); // start with Trig LOW
    }

    float MeasureDistanceCM();

private:
    void init();

    const HC_SRO4::Pins m_pins;

    const int m_timeOut = 30000;
};
