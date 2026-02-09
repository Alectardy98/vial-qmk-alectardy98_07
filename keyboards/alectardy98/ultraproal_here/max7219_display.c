#include "quantum.h"
#include "spi_master.h"

// ---------- User-configurable (overridable via config.h) ----------
#ifndef MAX7219_CS_PIN
#    define MAX7219_CS_PIN GP8   // You set this in config.h; this is a safe default
#endif

#ifndef MAX7219_NUM_DIGITS
// Set this to the *physical* digit count. Scanning unused digits reduces duty-cycle and makes the display look dim.
#    define MAX7219_NUM_DIGITS 4  // 1..8 supported by a single MAX7219
#endif

#ifndef MAX7219_INTENSITY_DEFAULT
// 0x00..0x0F. Use 0x0F for maximum brightness.
#    define MAX7219_INTENSITY_DEFAULT 0x0F
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

// ---------- 4-digit helpers (no mirroring) ----------
static void max7219_clear_decode_mode(void) {
    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS; d++) {
        max7219_write_digit(d, 0x0F); // blank
    }
}

// Render an unsigned number into the available digits using Code-B decode.
// Digit 0 is the least significant digit.
static void max7219_show_uint32_decode(uint32_t n) {
    if (n == 0) {
        max7219_write_digit(0, 0); // "0"
        for (uint8_t d = 1; d < MAX7219_NUM_DIGITS; d++) {
            max7219_write_digit(d, 0x0F);
        }
        return;
    }

    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS; d++) {
        if (n) {
            max7219_write_digit(d, (uint8_t)(n % 10));
            n /= 10;
        } else {
            max7219_write_digit(d, 0x0F);
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
    max7219_set_scan_limit((uint8_t)(MAX7219_NUM_DIGITS - 1)); // for 4 digits: 0x03
    // Enable Code-B decode only for the digits we physically have.
    // For 4 digits this becomes 0b00001111 = 0x0F.
    uint8_t decode_mask = (MAX7219_NUM_DIGITS >= 8) ? 0xFF : (uint8_t)((1u << MAX7219_NUM_DIGITS) - 1u);
    max7219_set_decode(decode_mask);
    max7219_set_intensity(MAX7219_INTENSITY_DEFAULT);
    max7219_clear_decode_mode();                               // blank all digits
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
        max7219_show_uint32_decode(counter++);
        // With 4 digits, keep to 0..9999 for clean display
        if (counter > 9999UL) counter = 0;
    }
}
