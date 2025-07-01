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

static void max7219_send16(uint8_t reg, uint8_t data) {
    writePin(MAX7219_LOAD_PIN, 0);
    for (int i = 0; i < 8; ++i) {
        writePin(MAX7219_CLK_PIN, 0);
        writePin(MAX7219_DIN_PIN, (reg & 0x80) ? 1 : 0);
        reg <<= 1;
        writePin(MAX7219_CLK_PIN, 1);
    }
    for (int i = 0; i < 8; ++i) {
        writePin(MAX7219_CLK_PIN, 0);
        writePin(MAX7219_DIN_PIN, (data & 0x80) ? 1 : 0);
        data <<= 1;
        writePin(MAX7219_CLK_PIN, 1);
    }
    writePin(MAX7219_LOAD_PIN, 1);
}

void max7219_init(void) {
    setPinOutput(MAX7219_CLK_PIN);
    setPinOutput(MAX7219_DIN_PIN);
    setPinOutput(MAX7219_LOAD_PIN);
    writePin(MAX7219_LOAD_PIN, 1);

    max7219_send16(0x09, 0xFF); // Decode mode: code B for all digits
    max7219_send16(0x0A, 0x0F); // Intensity: max
    max7219_send16(0x0B, 0x07); // Scan limit: all digits
    max7219_send16(0x0C, 0x01); // Shutdown: normal operation
    max7219_send16(0x0F, 0x00); // Display test: off
    for (uint8_t i = 1; i <= 8; ++i)
        max7219_send16(i, 0x0F); // Clear all digits
}

void max7219_set_digit(uint8_t pos, uint8_t value) {
    if (pos < 1 || pos > 8) return;
    max7219_send16(pos, value);
}