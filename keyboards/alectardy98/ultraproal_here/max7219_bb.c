#include "print.h"       // for xprintf()
#include "quantum.h"
#include "config.h"

// Pulse a pin (HIGH→LOW)
static inline void pulse(uint8_t pin) {
    writePinHigh(pin);
    writePinLow(pin);
}

// Send one 16-bit word (addr + data) MSB-first
static void max_write(uint8_t addr, uint8_t data) {
    writePinLow(MAX_CS_PIN);
    for (int8_t i = 7; i >= 0; i--) {
        writePinLow(MAX_CLK_PIN);
        if (addr & (1 << i)) writePinHigh(MAX_DIN_PIN);
        else                  writePinLow(MAX_DIN_PIN);
        pulse(MAX_CLK_PIN);
    }
    for (int8_t i = 7; i >= 0; i--) {
        writePinLow(MAX_CLK_PIN);
        if (data & (1 << i)) writePinHigh(MAX_DIN_PIN);
        else                  writePinLow(MAX_DIN_PIN);
        pulse(MAX_CLK_PIN);
    }
    pulse(MAX_CS_PIN);
}

// Write the same value to all 8 digit registers
static void max_fill(uint8_t value) {
    for (uint8_t digit = 1; digit <= 8; digit++) {
        max_write(digit, value);
    }
}

void keyboard_post_init_user(void) {
    // Configure and idle pins HIGH so float → pulled to +5 V
    setPinOutput(MAX_DIN_PIN);  writePinHigh(MAX_DIN_PIN);
    setPinOutput(MAX_CLK_PIN);  writePinHigh(MAX_CLK_PIN);
    setPinOutput(MAX_CS_PIN);   writePinHigh(MAX_CS_PIN);

    // MAX7219 init sequence:
    max_write(0x09, 0x00); // Decode: none
    max_write(0x0B, 0x07); // Scan: digits 0–7
    max_write(0x0A, 0x08); // Intensity: mid
    max_write(0x0C, 0x01); // Normal operation
    max_write(0x0F, 0x00); // Display-test: off

    // Clear display
    max_fill(0x00);

    xprintf("MAX7219 init complete\n");
}

void matrix_scan_user(void) {
    static uint32_t last_ms;
    static bool     sink;

    if (last_ms == 0) {
        last_ms = timer_read();
        xprintf("Starting blink timer\n");
    }

    // Every 1000 ms, toggle sink state
    if (timer_elapsed(last_ms) > 1000) {
        last_ms = timer_read();
        sink    = !sink;
        xprintf("Blink: sink=%d (byte=0x%02X)\n", sink, sink ? 0xFF : 0x00);
        // sink==true  → 0xFF → MAX7219 sinks all segments to GND
        // sink==false → 0x00 → MAX7219 floats all segments → pulled to +5 V
        max_fill(sink ? 0xFF : 0x00);
    }
}  // <-- closing brace for matrix_scan_user
