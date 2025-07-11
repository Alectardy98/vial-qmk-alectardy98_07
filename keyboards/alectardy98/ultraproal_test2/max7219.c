#include "quantum.h"
#include "config.h"

// Bit-bang one 16-bit word into MAX7219 (addr + data)
static void max7219_send_pair(uint8_t addr, uint8_t val) {
    // CS low
    writePinLow(MAX7219_LOAD_PIN);
    // shift out 16 bits MSB first
    for (int8_t i = 15; i >= 0; i--) {
        writePin(MAX7219_CLK_PIN, false);
        // top byte is addr, bottom is val
        bool bit = (i >= 8)
            ? ((addr >> (i-8)) & 1)
            : ((val  >> (i  )) & 1);
        writePin(MAX7219_DATA_PIN, bit);
        writePin(MAX7219_CLK_PIN, true);
    }
    // CS high
    writePinHigh(MAX7219_LOAD_PIN);
}

// Set up decode mode, scan limit, intensity, turn on
void max7219_init(void) {
    setPinOutput(MAX7219_LOAD_PIN);
    setPinOutput(MAX7219_DATA_PIN);
    setPinOutput(MAX7219_CLK_PIN);
    // Decode all digits
    max7219_send_pair(0x09, 0xFF);
    // Scan limit = 0…7 (we have 8 digits)
    max7219_send_pair(0x0B, MAX7219_NUM_DIGITS - 1);
    // Normal operation
    max7219_send_pair(0x0C, 0x01);
    // Intensity (0x00..0x0F)
    max7219_send_pair(0x0A, 0x07);
    // Clear all to 0
    for (uint8_t d = 1; d <= MAX7219_NUM_DIGITS; d++) {
        max7219_send_pair(d, 0x0);
    }
}

// Display a single digit (1…8), value 0x0…0xF (0…F). For blank, use 0xF if decode off
void max7219_set_digit(uint8_t pos, uint8_t value) {
    if (pos < 1 || pos > MAX7219_NUM_DIGITS) return;
    max7219_send_pair(pos, value);
}
