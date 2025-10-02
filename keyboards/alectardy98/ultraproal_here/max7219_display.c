#include "quantum.h"
#include "spi_master.h"

// ---------- User-configurable pins (override in config.h if you want) ----------
#ifndef MAX7219_CS_PIN
#    define MAX7219_CS_PIN GP17   // Chip Select / LOAD
#endif

#ifndef MAX7219_NUM_DIGITS
#    define MAX7219_NUM_DIGITS 8  // MAX7219 supports 1..8 digits
#endif

#ifndef MAX7219_INTENSITY_DEFAULT
#    define MAX7219_INTENSITY_DEFAULT 0x0C  // 0x00..0x0F (a bit bright so it's obvious)
#endif

// ---------- MAX7219 register map ----------
#define REG_NOOP      0x00
#define REG_DIGIT0    0x01  // up to 0x08
#define REG_DECODE    0x09
#define REG_INTENSITY 0x0A
#define REG_SCANLIM   0x0B
#define REG_SHUTDOWN  0x0C
#define REG_TEST      0x0F

// ---------- Low-level TX (hardware SPI, mode 0) ----------
static inline void max7219_tx(uint8_t reg, uint8_t data) {
    // lsb_first=false, mode=0, divisor=128 (nice and safe for bring-up)
    spi_start(MAX7219_CS_PIN, false, 0, 128);
    spi_write(reg);
    spi_write(data);
    spi_stop();
}

// ---------- Helpers ----------
static inline void max7219_set_intensity(uint8_t i) {
    max7219_tx(REG_INTENSITY, i & 0x0F);
}

static inline void max7219_set_scan_limit(uint8_t digits_minus_1) {
    if (digits_minus_1 > 7) digits_minus_1 = 7;
    max7219_tx(REG_SCANLIM, digits_minus_1);
}

// mask: bit per digit for Code-B decode (0=raw, 1=decode). 0xFF = decode on all digits.
static inline void max7219_set_decode(uint8_t mask) {
    max7219_tx(REG_DECODE, mask);
}

// off=true enters shutdown, off=false normal (on)
static inline void max7219_shutdown(bool off) {
    max7219_tx(REG_SHUTDOWN, off ? 0x00 : 0x01);
}

// Write a digit register (0..MAX7219_NUM_DIGITS-1)
// In **decode mode**, data values:
// 0..9 = digit, 0x0A..0x0E = dash/letters, 0x0F = blank
static inline void max7219_write_digit(uint8_t digit, uint8_t val) {
    if (digit >= MAX7219_NUM_DIGITS) return;
    max7219_tx((uint8_t)(REG_DIGIT0 + digit), val);
}

static void max7219_clear_decode_mode(void) {
    // In decode mode, "blank" is 0x0F
    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS; d++) {
        max7219_write_digit(d, 0x0F);
    }
}

// Render an unsigned number using decode mode (least significant at digit 0).
static void max7219_show_uint32_decode(uint32_t n) {
    // If zero, explicitly show 0 once, blanks elsewhere
    if (n == 0) {
        max7219_write_digit(0, 0); // "0"
        for (uint8_t d = 1; d < MAX7219_NUM_DIGITS; d++) max7219_write_digit(d, 0x0F);
        return;
    }
    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS; d++) {
        if (n) {
            uint8_t digit = (uint8_t)(n % 10);
            max7219_write_digit(d, digit);  // 0..9
            n /= 10;
        } else {
            max7219_write_digit(d, 0x0F);   // blank
        }
    }
}

// ---------- Public API ----------
void max7219_init(void) {
    spi_init();

    // Optional “all on” flash to prove wiring; comment out if you like
    max7219_tx(REG_TEST, 0x01);  // display test (all segments)
    wait_ms(150);
    max7219_tx(REG_TEST, 0x00);

    max7219_shutdown(false);                                // on
    max7219_set_scan_limit((uint8_t)(MAX7219_NUM_DIGITS - 1)); // digits 0..N-1
    max7219_set_decode(0xFF);                               // use Code-B decode on all digits
    max7219_set_intensity(MAX7219_INTENSITY_DEFAULT);
    max7219_clear_decode_mode();                            // blanks
}

// Keep your raw API available if you need it elsewhere
void max7219_set_digit_raw(uint8_t digit, uint8_t segmask) {
    // If you want raw segments, switch decode off first: max7219_set_decode(0x00);
    max7219_write_digit(digit, segmask);
}

void max7219_enable_hex_decode(bool enable_all_digits) {
    max7219_set_decode(enable_all_digits ? 0xFF : 0x00);
}

// ---------- AUTO-COUNTER DEMO (no key input required) ----------
// We implement keyboard_post_init_user + matrix_scan_user here so it “just works”.
// If your keymap already defines these, either:
//  - move the code into your existing functions, or
//  - rename these to max7219_post_init() / max7219_task() and call them from there.

void keyboard_post_init_user(void) {
    max7219_init();
}

// Update every ~200 ms
void matrix_scan_user(void) {
    static uint32_t last = 0;
    static uint32_t counter = 0;

    uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, last) >= 200) {
        last = now;
        max7219_show_uint32_decode(counter++);
        if (counter > 99999999UL) counter = 0; // wrap at 8 digits
    }
}
