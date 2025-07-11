#include "quantum.h"
#include "bar_led_74hc595.h"

#ifndef HC595_SER_PIN
#    error "HC595_SER_PIN not defined!"
#endif
#ifndef HC595_SRCLK_PIN
#    error "HC595_SRCLK_PIN not defined!"
#endif
#ifndef HC595_RCLK_PIN
#    error "HC595_RCLK_PIN not defined!"
#endif

static uint8_t bar_led_state = 0;

// low-level write of all 8 bits to the 74HC595
void bar_led_write(uint8_t val) {
    // make sure our pins are outputs
    setPinOutput(HC595_SER_PIN);
    setPinOutput(HC595_SRCLK_PIN);
    setPinOutput(HC595_RCLK_PIN);

    // latch low
    writePin(HC595_RCLK_PIN, 0);
    // shift MSB first
    for (int8_t bit = 7; bit >= 0; --bit) {
        writePin(HC595_SRCLK_PIN, 0);
        writePin(HC595_SER_PIN, (val >> bit) & 1);
        writePin(HC595_SRCLK_PIN, 1);
    }
    // latch high to push data out
    writePin(HC595_RCLK_PIN, 1);
}

void bar_led_init(void) {
    bar_led_state = 0;
    bar_led_write(bar_led_state);
}

void bar_led_set(uint8_t bar, bool on) {
    if (bar > 7) return;
    if (on) {
        bar_led_state |= (1 << bar);
    } else {
        bar_led_state &= ~(1 << bar);
    }
    bar_led_write(bar_led_state);
}

void bar_led_toggle(uint8_t bar) {
    if (bar > 7) return;
    bar_led_state ^= (1 << bar);
    bar_led_write(bar_led_state);
}
