#include "quantum.h"
#include "spi_master.h"

// ---------- User-configurable (overridable via config.h) ----------
#ifndef MAX7219_CS_PIN
#    define MAX7219_CS_PIN GP8   // You set this in config.h; this is a safe default
#endif

#ifndef MAX7219_NUM_DIGITS
#    define MAX7219_NUM_DIGITS 8  // 1..8 supported by a single MAX7219
#endif

#ifndef MAX7219_INTENSITY_DEFAULT
#    define MAX7219_INTENSITY_DEFAULT 0x0C  // 0x00..0x0F
#endif

// ---------- MAX7219 register map ----------
#define REG_NOOP      0x00
#define REG_DIGIT0    0x01  // through 0x08
#define REG_DECODE    0x09
#define REG_INTENSITY 0x0A
#define REG_SCANLIM   0x0B
#define REG_SHUTDOWN  0x0C
#define REG_TEST      0x0F

// ---------- Low-level TX (hardware SPI, mode 0) ----------
// MSB-first, mode 0, divisor 128 (safe for bring-up). Uses SPI pins from config.h:
//   SPI_DRIVER=SPID0, MOSI=GP7, SCK=GP6, MISO=NO_PIN (write-only).
static inline void max7219_tx(uint8_t reg, uint8_t data) {
    spi_start(MAX7219_CS_PIN, /*lsb_first=*/false, /*mode=*/0, /*divisor=*/128);
    spi_write(reg);
    spi_write(data);
    spi_stop(); // latch
}

// ---------- Helpers ----------
static inline void max7219_set_intensity(uint8_t i) {
    max7219_tx(REG_INTENSITY, i & 0x0F);
}

static inline void max7219_set_scan_limit(uint8_t digits_minus_1) {
    if (digits_minus_1 > 7) digits_minus_1 = 7;
    max7219_tx(REG_SCANLIM, digits_minus_1);
}

// mask bit per digit for Code-B decode (0=raw, 1=decode). 0xFF = decode on all 8.
static inline void max7219_set_decode(uint8_t mask) {
    max7219_tx(REG_DECODE, mask);
}

// off=true enters shutdown; off=false normal operation
static inline void max7219_shutdown(bool off) {
    max7219_tx(REG_SHUTDOWN, off ? 0x00 : 0x01);
}

// Write a digit register (0..MAX7219_NUM_DIGITS-1)
// In decode mode: 0..9 digits, 0x0F blank
static inline void max7219_write_digit(uint8_t digit, uint8_t val) {
    if (digit >= MAX7219_NUM_DIGITS) return;
    max7219_tx((uint8_t)(REG_DIGIT0 + digit), val);
}

// ---------- Mirror helpers (mirror 0..(N/2-1) onto (N/2)..(N-1)) ----------
static inline uint8_t max7219_partner(uint8_t d) {
    uint8_t half = MAX7219_NUM_DIGITS / 2;
    // For odd digit counts, partner==d for the center (we only write once).
    return (d < MAX7219_NUM_DIGITS) ? (uint8_t)((d + half) % MAX7219_NUM_DIGITS) : d;
}

static inline void max7219_write_digit_mirrored(uint8_t d, uint8_t val) {
    if (d >= MAX7219_NUM_DIGITS) return;
    max7219_write_digit(d, val);
    uint8_t p = max7219_partner(d);
    if (p != d) max7219_write_digit(p, val);
}

static void max7219_clear_decode_mode_mirrored(void) {
    uint8_t half = MAX7219_NUM_DIGITS / 2;
    for (uint8_t d = 0; d < half; d++) {
        max7219_write_digit_mirrored(d, 0x0F); // blank both halves
    }
}

// Render an unsigned number mirrored across halves.
// With 8 digits, we show up to 4 LSBs in 0..3 and copy to 4..7.
static void max7219_show_uint32_decode_mirrored(uint32_t n) {
    uint8_t half = MAX7219_NUM_DIGITS / 2;

    if (n == 0) {
        max7219_write_digit_mirrored(0, 0);   // "0"
        for (uint8_t d = 1; d < half; d++) {
            max7219_write_digit_mirrored(d, 0x0F);
        }
        return;
    }

    for (uint8_t d = 0; d < half; d++) {
        if (n) {
            uint8_t digit = (uint8_t)(n % 10);
            max7219_write_digit_mirrored(d, digit);
            n /= 10;
        } else {
            max7219_write_digit_mirrored(d, 0x0F);
        }
    }
}

// ---------- Public API ----------
void max7219_init(void) {
    // Configure SPI driver/pins per config.h (SPID0, GP7 MOSI, GP6 SCK, NO_PIN MISO)
    spi_init();

    // Optional “all on” flash to prove wiring
    max7219_tx(REG_TEST, 0x01);
    wait_ms(150);
    max7219_tx(REG_TEST, 0x00);

    max7219_shutdown(false);                                   // turn on
    max7219_set_scan_limit((uint8_t)(MAX7219_NUM_DIGITS - 1)); // digits 0..N-1
    max7219_set_decode(0xFF);                                  // Code-B on all digits
    max7219_set_intensity(MAX7219_INTENSITY_DEFAULT);
    max7219_clear_decode_mode_mirrored();                      // blanks both halves

    // (Optional) sanity stamp to confirm init ran:
    // max7219_write_digit_mirrored(0, 1);
    // max7219_write_digit_mirrored(1, 2);
    // max7219_write_digit_mirrored(2, 3);
    // max7219_write_digit_mirrored(3, 4);
}

// Raw segment write (use after max7219_set_decode(0x00))
void max7219_set_digit_raw(uint8_t digit, uint8_t segmask) {
    max7219_write_digit(digit, segmask);
}

void max7219_enable_hex_decode(bool enable_all_digits) {
    max7219_set_decode(enable_all_digits ? 0xFF : 0x00);
}

// ---------- AUTO-COUNTER DEMO ----------
void keyboard_post_init_user(void) {
    max7219_init();
}

void matrix_scan_user(void) {
    static uint32_t last = 0;
    static uint32_t counter = 0;

    uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, last) >= 200) {
        last = now;
        max7219_show_uint32_decode_mirrored(counter++);
        // With 8 digits mirrored (4 per half), keep to 0..9999 for clean display
        if (counter > 9999UL) counter = 0;
    }
}
