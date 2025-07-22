#include "quantum.h"
#include "config.h"
#include <stdint.h>

// Pin aliases from config.h
#define CS_PIN   SPI_MOSI_PIN
#define CLK_PIN  SPI_SCK_PIN
#define DIN_PIN  MAX7219_DIN_PIN

// Pulse a pin high→low
static inline void bb_pulse(uint8_t pin) {
    writePinHigh(pin);
    writePinLow(pin);
}

// Shift out one byte, MSB first
static void bb_send_byte(uint8_t data) {
    for (int8_t b = 7; b >= 0; b--) {
        writePin(DIN_PIN, (data >> b) & 1);
        bb_pulse(CLK_PIN);
    }
}

// Send [register, data]
static void max7219_send(uint8_t reg, uint8_t val) {
    writePinLow(CS_PIN);
    bb_send_byte(reg);
    bb_send_byte(val);
    writePinHigh(CS_PIN);
}

// Initialize MAX7219 in bit-bang mode
void max7219_init(void) {
    setPinOutput(DIN_PIN);
    setPinOutput(CLK_PIN);
    setPinOutput(CS_PIN);
    writePinHigh(CS_PIN);

    wait_ms(50);

    // Configuration registers
    max7219_send(0x0F, 0x00);  // display test off
    max7219_send(0x0C, 0x01);  // normal operation (exit shutdown)
    max7219_send(0x0B, 0x03);  // scan limit: digits 1–4
    max7219_send(0x09, 0x00);  // no decode (raw)
    max7219_send(0x0A, 0x0F);  // intensity: max

    // Initially clear all digits
    for (uint8_t d = 1; d <= 4; d++) {
        max7219_send(d, 0x00);
    }
}

void keyboard_post_init_user(void) {
    max7219_init();
}

// Flash all segments on/off every 500 ms
void keyboard_task_user(void) {
    static uint32_t last = 0;
    static bool on = false;

    uint32_t now = timer_read32();
    if ((uint32_t)(now - last) >= 500) {
        last = now;
        on = !on;

        uint8_t pattern = on ? 0xFF : 0x00;
        for (uint8_t d = 1; d <= 4; d++) {
            max7219_send(d, pattern);
        }
    }
}
