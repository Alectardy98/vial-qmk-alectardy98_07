/* SPI 74HC595 per-LED multiplexing (ALL 24 LEDs ON)
 * ATmega32U4 pins: B2 MOSI, B1 SCK, B3 LATCH, PB0 forced HIGH (SS)
 */
#include QMK_KEYBOARD_H
#include <avr/io.h>

// ---------------- Pins ----------------
#define DATA_PIN    B2   // MOSI (PB2)
#define CLK_PIN     B1   // SCK  (PB1)
#define LATCH_PIN   B3   // RCLK/STCP (PB3)

// 0 = active-high LEDs (your case); set to 1 for active-low (595 sinking)
#ifndef SHIFT_ACTIVE_LOW
#    define SHIFT_ACTIVE_LOW 0
#endif
// Keep SS (PB0) high so SPI never drops to slave mode
#ifndef FORCE_SS_HIGH
#    define FORCE_SS_HIGH 1
#endif
// Per-LED dwell; 0 = advance every housekeeping call (fastest, no flicker)
#ifndef PHASE_PERIOD_MS
#    define PHASE_PERIOD_MS 0
#endif

// ---------------- LED bit names (0..23 in correct order) ----------------
#define LED_AB_PREVIEW       (1UL << 0)
#define LED_AB_MIX           (1UL << 1)
#define LED_CH_B_DISPLAY     (1UL << 2)
#define LED_CH_B_6           (1UL << 3)
#define LED_CH_B_4           (1UL << 4)
#define LED_CH_B_2           (1UL << 5)
#define LED_CH_B_1           (1UL << 6)
#define LED_CH_B_3           (1UL << 7)
#define LED_CH_B_5           (1UL << 8)
#define LED_CH_A_DISPLAY     (1UL << 9)
#define LED_CH_A_6           (1UL << 10)
#define LED_CH_A_4           (1UL << 11)
#define LED_CH_A_2           (1UL << 12)
#define LED_CH_A_1           (1UL << 13)
#define LED_CH_A_3           (1UL << 14)
#define LED_CH_A_5           (1UL << 15)
#define LED_KEYBOARD_DISPLAY (1UL << 16)
#define LED_KEYBOARD_6       (1UL << 17)
#define LED_KEYBOARD_4       (1UL << 18)
#define LED_KEYBOARD_2       (1UL << 19)
#define LED_KEYBOARD_1       (1UL << 20)
#define LED_KEYBOARD_3       (1UL << 21)
#define LED_KEYBOARD_5       (1UL << 22)
#define LED_QB               (1UL << 23)

// Keep layers so keymap compiles
enum { _BASE = 0, _FN };

// ---------------- SPI helpers ----------------
static inline void spi_hw_init(void) {
    DDRB |= _BV(PB2) | _BV(PB1);     // MOSI, SCK outputs
#if FORCE_SS_HIGH
    DDRB  |= _BV(PB0);
    PORTB |= _BV(PB0);               // SS high = lock master mode
#endif
    setPinOutput(LATCH_PIN);
    writePinLow(LATCH_PIN);

    // SPI master, CPOL=0, CPHA=0, Fosc/16 (~1 MHz @ 16 MHz)
    SPCR = _BV(SPE) | _BV(MSTR) | _BV(SPR0);
    // Optional: faster updates (uncomment if wiring solid)
    // SPSR |= _BV(SPI2X);
}

static inline void spi_hw_write(uint8_t b) {
    SPDR = b;
    while (!(SPSR & _BV(SPIF))) {}
}

static inline void shift595_write_raw(uint32_t phys_bits) {
#if SHIFT_ACTIVE_LOW
    phys_bits = ~phys_bits;
#endif
    spi_hw_write((phys_bits >> 16) & 0xFF);
    spi_hw_write((phys_bits >> 8)  & 0xFF);
    spi_hw_write( phys_bits        & 0xFF);
    writePinLow(LATCH_PIN);
    writePinHigh(LATCH_PIN);
}

// ---------------- Multiplexing ----------------
static uint8_t  phase = 0;           // 0..23
static uint32_t last_phase_tick = 0;

// All 24 LEDs should appear ON
static inline uint32_t logical_on_mask(void) {
    return 0xFFFFFFUL;   // bits 0..23 set
}

// Drive only one LED at this phase
static inline uint32_t one_led_frame(uint8_t idx, uint32_t mask) {
    uint32_t bit = (1UL << idx);
    return (mask & bit) ? bit : 0;
}

// ---------------- QMK hooks ----------------
void keyboard_pre_init_kb(void) {
    spi_hw_init();
    last_phase_tick = timer_read32();

    uint32_t frame = one_led_frame(phase, logical_on_mask());
    shift595_write_raw(frame);
}

void housekeeping_task_kb(void) {
#if PHASE_PERIOD_MS > 0
    if (timer_elapsed32(last_phase_tick) < PHASE_PERIOD_MS) return;
    last_phase_tick = timer_read32();
#endif
    phase = (uint8_t)((phase + 1) % 24);
    uint32_t frame = one_led_frame(phase, logical_on_mask());
    shift595_write_raw(frame);
}

// ---------------- Minimal keymaps (placeholders) ----------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                       _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,              _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                _______,
        _______, _______, _______, _______,          _______,                                     _______,                                     _______,                                _______, _______, _______
    ),
    [_FN] = LAYOUT(
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                       _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,              _______, _______, _______,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                                _______,
        _______, _______, _______, _______,          _______,                                     _______,                                     _______,                                _______, _______, _______
    ),
};
