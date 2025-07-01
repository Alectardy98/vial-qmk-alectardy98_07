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

void bar_led_init(void) {
    setPinOutput(HC595_SER_PIN);
    setPinOutput(HC595_SRCLK_PIN);
    setPinOutput(HC595_RCLK_PIN);
    bar_led_write(0);
}

void bar_led_write(uint8_t val) {
    bar_led_state = val;
    for (int8_t i = 7; i >= 0; --i) {
        writePin(HC595_SRCLK_PIN, 0);
        writePin(HC595_SER_PIN, (val >> i) & 1);
        writePin(HC595_SRCLK_PIN, 1);
    }
    writePin(HC595_RCLK_PIN, 0);
    writePin(HC595_RCLK_PIN, 1);
}

void bar_led_set(uint8_t bar, bool on) {
    if (on)
        bar_led_state |= (1 << bar);
    else
        bar_led_state &= ~(1 << bar);
    bar_led_write(bar_led_state);
}

void bar_led_toggle(uint8_t bar) {
    bar_led_state ^= (1 << bar);
    bar_led_write(bar_led_state);
}