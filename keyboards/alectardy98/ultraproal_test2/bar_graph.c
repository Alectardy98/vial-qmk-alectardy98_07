#include "quantum.h"
#include "config.h"

// Pulse RCLK to latch 8 bits
static void bar_latch(void) {
    writePinHigh(BAR_SR_RCLK_PIN);
    __asm__ volatile("nop");   // one CPU cycle delay
    writePinLow(BAR_SR_RCLK_PIN);
}

// Shift out one byte, MSB first
static void bar_shift_byte(uint8_t data) {
    for (int8_t i = 7; i >= 0; i--) {
        writePinLow(BAR_SR_SRCLK_PIN);
        writePin(BAR_SR_SER_PIN, (data >> i) & 1);
        writePinHigh(BAR_SR_SRCLK_PIN);
    }
}

// User‐callable: set 10-segment bar (0..10)
void bar_set_level(uint8_t lvl) {
    writePin(BAR1_PIN, lvl > 0);
    writePin(BAR2_PIN, lvl > 1);
    uint8_t bits = 0;
    for (uint8_t b = 0; b < 8; b++) {
        if (lvl > (2 + b)) bits |= (1 << (7 - b));
    }
    bar_shift_byte(bits);
    bar_latch();
}

void bar_graph_init(void) {
    setPinOutput(BAR_SR_SER_PIN);
    setPinOutput(BAR_SR_SRCLK_PIN);
    setPinOutput(BAR_SR_RCLK_PIN);
    setPinOutput(BAR1_PIN);
    setPinOutput(BAR2_PIN);
    bar_set_level(0);
}
