#include "quantum.h"
#include "config.h"

// Pulse a pin HIGH→LOW
static inline void sr_pulse(uint8_t pin) {
    writePinHigh(pin);
    writePinLow(pin);
}

// Send & latch one byte (MSB first) to the 74HC595
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
    static uint32_t last_ms;
    static uint8_t  idx;

    // Stamp the first timestamp
    if (last_ms == 0) {
        last_ms = timer_read();
    }

    // Every 200 ms, advance the “pixel”
    if (timer_elapsed(last_ms) > 200) {
        last_ms = timer_read();

        // Light one LED at a time, moving down the bar:
        // bit 7 → first LED, bit 0 → last LED
        bar_led_write(1 << (7 - idx));

        // Advance and wrap 0–7
        idx = (idx + 1) & 0x07;
    }
}
