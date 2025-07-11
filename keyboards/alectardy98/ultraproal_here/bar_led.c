#include "quantum.h"
#include "config.h"

// Pulse a pin high→low
static inline void sr_pulse(uint8_t pin) {
    writePinHigh(pin);
    writePinLow(pin);
}

// Send & latch one byte (MSB first)
void bar_led_write(uint8_t bits) {
    // 1) Hold latch low while shifting
    writePinLow(BAR_RCLK_PIN);

    // 2) Shift out 8 bits, MSB first
    for (int8_t i = 7; i >= 0; i--) {
        if (bits & (1 << i)) {
            writePinHigh(BAR_SER_PIN);
        } else {
            writePinLow(BAR_SER_PIN);
        }
        sr_pulse(BAR_SRCLK_PIN);
    }

    // 3) Pulse latch to update outputs
    sr_pulse(BAR_RCLK_PIN);
}

void keyboard_post_init_user(void) {
    // Configure SER, SRCLK, RCLK as outputs
    setPinOutput(BAR_SER_PIN);
    setPinOutput(BAR_SRCLK_PIN);
    setPinOutput(BAR_RCLK_PIN);

    // Clear all LEDs on startup
    bar_led_write(0x00);
}

void matrix_scan_user(void) {
    static uint16_t last_timer;
    static bool on;

    if (timer_elapsed(last_timer) > 500) {
        last_timer = timer_read();
        on = !on;
        // 0xFF = all LEDs on, 0x00 = all off
        bar_led_write(on ? 0xFF : 0x00);
    }
}
