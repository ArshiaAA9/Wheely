#pragma once
#include "soc/gpio_num.h"

struct TB6612Pins {
    gpio_num_t AIN1; // used for the directions
    gpio_num_t AIN2;
    gpio_num_t PWMA; // used for speed

    gpio_num_t BIN1;
    gpio_num_t BIN2;
    gpio_num_t PWMB;

    gpio_num_t STBY; // used to see if its in standby or active

    TB6612Pins() = default;
};

class TB6612 {
public:
    enum struct MotionState {
        Forward,
        Reverse,

        TurnRight,
        TurnLeft,

        Break,
        Coast,

    };

    // movement methods
    void Forward();
    void Reverse();
    void TurnRight();
    void TurnLeft();

    void setSpeedToMax();

    TB6612(const TB6612Pins& pins)
        : m_pins(pins) {
        init();
    }

    ~TB6612() {}

private:
    void init();

    TB6612Pins m_pins;
    MotionState m_state = MotionState::Coast;
    static constexpr int m_MAX_DUTY = 190; // ~75% of 255: keeps the 6V motors safe on a 7-8V pack
};
