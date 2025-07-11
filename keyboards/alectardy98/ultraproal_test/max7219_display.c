#include "quantum.h"
#include "max7219_display.h"

#ifndef MAX7219_CLK_PIN
#    error "MAX7219_CLK_PIN not defined!"
#endif
#ifndef MAX7219_DIN_PIN
#    error "MAX7219_DIN_PIN not defined!"
#endif
#ifndef MAX7219_LOAD_PIN
#    error "MAX7219_LOAD_PIN not defined!"
#endif

// send 16 bits: 8-bit register address, then 8-bit data
static void max7219_send16(uint8_t reg, uint8_t data) {
    // ensure pins are outputs
    setPinOutput(MAX7219_CLK_PIN);
    setPinOutput(MAX7219_DIN_PIN);
    setPinOutput(MAX7219_LOAD_PIN);

    // bring LOAD low to start
    writePin(MAX7219_LOAD_PIN, 0);

    // clock out register (MSB first)
    for (int8_t bit = 7; bit >= 0; --bit) {
        writePin(MAX7219_CLK_PIN, 0);
        writePin(MAX7219_DIN_PIN, (reg >> bit) & 1);
        writePin(MAX7219_CLK_PIN, 1);
    }
    // clock out data (MSB first)
    for (int8_t bit = 7; bit >= 0; --bit) {
        writePin(MAX7219_CLK_PIN, 0);
        writePin(MAX7219_DIN_PIN, (data >> bit) & 1);
        writePin(MAX7219_CLK_PIN, 1);
    }

    // latch it in
    writePin(MAX7219_LOAD_PIN, 1);
}

void max7219_init(void) {
    setPinOutput(MAX7219_CLK_PIN);
    setPinOutput(MAX7219_DIN_PIN);
    setPinOutput(MAX7219_LOAD_PIN);

    // turn off any test or shutdown modes and clear
    max7219_send16(0x09, 0xFF); // Decode mode: Code B on all digits
    max7219_send16(0x0A, 0x0F); // Intensity: max
    max7219_send16(0x0B, 0x07); // Scan limit: digits 0–7
    max7219_send16(0x0C, 0x01); // Exit shutdown
    max7219_send16(0x0F, 0x00); // No display test
    // clear all digits
    for (uint8_t i = 1; i <= 8; ++i) {
        max7219_send16(i, 0x0F);
    }
}

void max7219_set_digit(uint8_t pos, uint8_t value) {
    if (pos < 1 || pos > 8) return;
    max7219_send16(pos, value);
}
