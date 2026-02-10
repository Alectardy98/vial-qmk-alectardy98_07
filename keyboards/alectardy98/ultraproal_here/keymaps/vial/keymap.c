/* Copyright 2022 Alectardy98
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "timer.h"
#include "config.h"

// MAX7219 SPI
#include "quantum.h"
#include "spi_master.h"

/* ─────────────────────────────────────────────
 * Layers
 * ───────────────────────────────────────────── */
enum _layer {
    _BASE,
    _FN
};

/* ─────────────────────────────────────────────
 * Vial-style user macro keycodes + handler
 * ───────────────────────────────────────────── */
enum blender_keycode {
    LEDT = QK_KB_0, // put this on a key in Vial
};

/* ─────────────────────────────────────────────
 * Display modes
 * ───────────────────────────────────────────── */
typedef enum {
    MODE_DEFAULT = 0, // BOTH displays OFF
    MODE_TEST    = 1, // Bar + MAX7219 test patterns
} display_mode_t;

static display_mode_t g_mode = MODE_DEFAULT;

/* ─────────────────────────────────────────────
 * 74HC595 helpers
 * ───────────────────────────────────────────── */

static inline void sr_pulse(pin_t pin) {
    writePinHigh(pin);
    writePinLow(pin);
}

static void bar_led_write(uint8_t bits) {
    writePinLow(BAR_RCLK_PIN);

    for (int8_t i = 7; i >= 0; i--) {
        if (bits & (1 << i)) writePinHigh(BAR_SER_PIN);
        else                 writePinLow(BAR_SER_PIN);
        sr_pulse(BAR_SRCLK_PIN);
    }

    sr_pulse(BAR_RCLK_PIN);
}

/* ─────────────────────────────────────────────
 * Bar segments (ACTIVE-LOW)
 * ───────────────────────────────────────────── */

// Seg 1  -> GP5 (ON=LOW)
// Seg 2  -> GP4 (ON=LOW)
// Seg 3..10 -> 74HC595 QA..QH (bit0..bit7), ON=0
static uint8_t sr_state = 0xFF; // 1=OFF, 0=ON

static inline void sr_commit(void) { bar_led_write(sr_state); }

static inline void sr_set_bit(uint8_t bit, bool on) {
    if (on) sr_state &= ~(1u << bit);   // ON  -> 0
    else    sr_state |=  (1u << bit);   // OFF -> 1
    sr_commit();
}

static inline void segments_all_off(void) {
    writePinHigh(GP5); // seg1 OFF
    writePinHigh(GP4); // seg2 OFF
    sr_state = 0xFF;
    sr_commit();
}

// Turn all 10 segments ON (active-low)
static inline void segments_all_on(void) {
    writePinLow(GP5);  // seg1 ON
    writePinLow(GP4);  // seg2 ON
    sr_state = 0x00;   // QA..QH ON
    sr_commit();
}

static inline void segment_set(uint8_t seg, bool on) {
    switch (seg) {
        case 1: if (on) writePinLow(GP5); else writePinHigh(GP5); break;
        case 2: if (on) writePinLow(GP4); else writePinHigh(GP4); break;
        case 3:  sr_set_bit(0, on); break; // QA
        case 4:  sr_set_bit(1, on); break; // QB
        case 5:  sr_set_bit(2, on); break; // QC
        case 6:  sr_set_bit(3, on); break; // QD
        case 7:  sr_set_bit(4, on); break; // QE
        case 8:  sr_set_bit(5, on); break; // QF
        case 9:  sr_set_bit(6, on); break; // QG
        case 10: sr_set_bit(7, on); break; // QH
        default: break;
    }
}

/* ─────────────────────────────────────────────
 * MAX7219 (SPI)
 * Brightness is configured ONLY via config.h:
 *   #define MAX7219_INTENSITY_DEFAULT 0x0F  (or 0x08 for ~half)
 * ───────────────────────────────────────────── */

#ifndef MAX7219_CS_PIN
#    define MAX7219_CS_PIN GP8
#endif

#ifndef MAX7219_NUM_DIGITS
#    define MAX7219_NUM_DIGITS 4
#endif

#ifndef MAX7219_INTENSITY_DEFAULT
#    define MAX7219_INTENSITY_DEFAULT 0x0F
#endif

#define REG_DIGIT0    0x01
#define REG_DECODE    0x09
#define REG_INTENSITY 0x0A
#define REG_SCANLIM   0x0B
#define REG_SHUTDOWN  0x0C
#define REG_TEST      0x0F

static inline void max7219_tx(uint8_t reg, uint8_t data) {
    spi_start(MAX7219_CS_PIN, /*lsb_first=*/false, /*mode=*/0, /*divisor=*/128);
    spi_write(reg);
    spi_write(data);
    spi_stop();
}

static inline void max7219_write_digit(uint8_t digit, uint8_t val) {
    if (digit >= MAX7219_NUM_DIGITS) return;
    max7219_tx((uint8_t)(REG_DIGIT0 + digit), val);
}

static inline void max7219_blank_all(void) {
    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS; d++) {
        max7219_write_digit(d, 0x0F); // blank in Code-B decode
    }
}

// Force "display on" state (normal operation + test off)
static inline void max7219_force_on(void) {
    max7219_tx(REG_SHUTDOWN, 0x01);
    max7219_tx(REG_TEST, 0x00);
}

void max7219_init(void) {
    spi_init();

    // Optional “all on” flash to prove wiring
    max7219_tx(REG_TEST, 0x01);
    wait_ms(150);
    max7219_tx(REG_TEST, 0x00);

    // Normal operation
    max7219_tx(REG_SHUTDOWN, 0x01);

    // Scan only the digits you physically have
    max7219_tx(REG_SCANLIM, (uint8_t)(MAX7219_NUM_DIGITS - 1));

    // Code-B decode enabled only for the digits you have
    uint8_t decode_mask = (MAX7219_NUM_DIGITS >= 8) ? 0xFF : (uint8_t)((1u << MAX7219_NUM_DIGITS) - 1u);
    max7219_tx(REG_DECODE, decode_mask);

    // Brightness is set once here from config.h
    max7219_tx(REG_INTENSITY, (MAX7219_INTENSITY_DEFAULT & 0x0F));

    max7219_blank_all();
}

/* ─────────────────────────────────────────────
 * Mode switching helper
 * (No brightness changes here—config.h only)
 * ───────────────────────────────────────────── */
static inline void set_display_mode(display_mode_t mode) {
    g_mode = mode;

    if (g_mode == MODE_DEFAULT) {
        segments_all_off();
        max7219_blank_all();
    }
}

/* ─────────────────────────────────────────────
 * Init hooks
 * ───────────────────────────────────────────── */
void keyboard_pre_init_user(void) {
    // Shift register pins
    setPinOutput(BAR_SER_PIN);
    setPinOutput(BAR_SRCLK_PIN);
    setPinOutput(BAR_RCLK_PIN);

    // Direct segment pins (active-low)
    setPinOutput(GP4); // seg 2
    setPinOutput(GP5); // seg 1

    segments_all_off();
    g_mode = MODE_DEFAULT;
}

void keyboard_post_init_user(void) {
    // Hardening: make sure CS is a driven output and stays HIGH when idle
    setPinOutput(MAX7219_CS_PIN);
    writePinHigh(MAX7219_CS_PIN);

    max7219_init();

    set_display_mode(MODE_DEFAULT); // ensure both displays OFF at boot
}

/* ─────────────────────────────────────────────
 * TEST MODE: bar display
 * Pass A: one ON at a time (1..10)
 * Pass B: all ON except one (1..10)
 * Loop A↔B
 * ───────────────────────────────────────────── */
void housekeeping_task_user(void) {
    if (g_mode != MODE_TEST) return;

    static uint32_t last_step = 0;
    static uint8_t seg = 1;
    static bool invert_pass = false; // false=single-on, true=all-but-one

    if (timer_elapsed(last_step) < 333) return;
    last_step = timer_read();

    if (!invert_pass) {
        segments_all_off();
        segment_set(seg, true);
    } else {
        segments_all_on();
        segment_set(seg, false);
    }

    seg++;
    if (seg > 10) {
        seg = 1;
        invert_pass = !invert_pass;
    }
}

/* ─────────────────────────────────────────────
 * TEST MODE: MAX7219 pattern
 * + when the full loop finishes, force display back ON
 * ───────────────────────────────────────────── */
void matrix_scan_user(void) {
    if (g_mode != MODE_TEST) return;

    #define MAX7219_STEP_MS 200

    static uint32_t last = 0;
    static uint8_t phase = 0; // 0..3 single digit, 4 all digits
    static uint8_t val = 0;   // 0..9

    uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, last) < MAX7219_STEP_MS) return;
    last = now;

    // Compute desired digit outputs; write all digits every tick.
    uint8_t out[4] = {0x0F, 0x0F, 0x0F, 0x0F};

    if (phase < 4) {
        if (phase < MAX7219_NUM_DIGITS) out[phase] = val;
    } else {
        for (uint8_t d = 0; d < MAX7219_NUM_DIGITS; d++) out[d] = val;
    }

    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS; d++) {
        max7219_write_digit(d, out[d]);
    }

    // Advance state machine
    val++;
    if (val > 9) {
        val = 0;
        phase++;
        if (phase > 4) {
            // Completed the full loop (0..4). Wrap back to 0 and "force on".
            phase = 0;

            // Force display back ON (normal op + test off)
            max7219_force_on();
        }
    }
}

/* ─────────────────────────────────────────────
 * Macro handler
 * ───────────────────────────────────────────── */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    switch (keycode) {
        case LEDT:
            // Toggle between DEFAULT (all off) and TEST mode
            if (g_mode == MODE_TEST) set_display_mode(MODE_DEFAULT);
            else                    set_display_mode(MODE_TEST);
            return false;
    }
    return true;
}

/* ─────────────────────────────────────────────
 * Keymaps
 * ───────────────────────────────────────────── */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[_BASE] = LAYOUT(
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                                                                   LEDT),

[_FN] = LAYOUT(
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                                                                   _______),
};

const uint8_t music_map[MATRIX_ROWS][MATRIX_COLS] = LAYOUT(
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,  0,  0,  0,
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
                              0
);
