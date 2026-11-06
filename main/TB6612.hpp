#pragma once
#include "soc/gpio_num.h"

class TB6612 {
public:
    struct Pins {
        gpio_num_t AIN1; // used for the directions
        gpio_num_t AIN2;
        gpio_num_t PWMA; // used for speed

        gpio_num_t BIN1;
        gpio_num_t BIN2;
        gpio_num_t PWMB;

        gpio_num_t STBY; // used to see if its in standby or active
    };
    enum struct MotionState {
        Forward,
        Reverse,

        TurnRight,
        TurnLeft,

        Brake,
        Coast,

    };

    // movement methods
    void Forward();
    void Reverse();
    void TurnRight();
    void TurnLeft();
    void Brake();
    void Coast();

    void SetSpeedToMax();

    TB6612(const TB6612::Pins& pins)
        : m_pins(pins) {
        init();
    }

    ~TB6612() {}

private:
    void init();

    const TB6612::Pins m_pins;
    MotionState m_state = MotionState::Coast;
    static constexpr int m_MAX_DUTY = 190; // ~75% of 255: keeps the 6V motors safe on a 7-8V pack
};
