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

enum _layer {
    _BASE,
    _FN
};

/* ─────────────────────────────────────────────
 * 74HC595 helpers
 * ───────────────────────────────────────────── */

// Pulse a pin HIGH → LOW
static inline void sr_pulse(pin_t pin) {
    writePinHigh(pin);
    writePinLow(pin);
}

// Send & latch one byte (MSB first) to the 74HC595
static void bar_led_write(uint8_t bits) {
    // Hold latch low while shifting
    writePinLow(BAR_RCLK_PIN);

    // Shift out 8 bits, MSB first
    for (int8_t i = 7; i >= 0; i--) {
        if (bits & (1 << i)) {
            writePinHigh(BAR_SER_PIN);
        } else {
            writePinLow(BAR_SER_PIN);
        }
        sr_pulse(BAR_SRCLK_PIN);
    }

    // Latch outputs
    sr_pulse(BAR_RCLK_PIN);
}

/* ─────────────────────────────────────────────
 * Segment labeling + modes (ACTIVE-LOW)
 * ───────────────────────────────────────────── */

// 10-segment mapping (ACTIVE-LOW)
//
// Seg 1  -> GP5         (direct GPIO)  ON=LOW
// Seg 2  -> GP4         (direct GPIO)  ON=LOW
// Seg 3  -> 74HC595 QA  (bit 0)        ON=0
// Seg 4  -> 74HC595 QB  (bit 1)        ON=0
// Seg 5  -> 74HC595 QC  (bit 2)        ON=0
// Seg 6  -> 74HC595 QD  (bit 3)        ON=0
// Seg 7  -> 74HC595 QE  (bit 4)        ON=0
// Seg 8  -> 74HC595 QF  (bit 5)        ON=0
// Seg 9  -> 74HC595 QG  (bit 6)        ON=0
// Seg 10 -> 74HC595 QH  (bit 7)        ON=0

// ACTIVE-LOW state: 1 = OFF, 0 = ON
static uint8_t sr_state = 0xFF; // QA..QH all OFF at boot

static inline void sr_commit(void) {
    bar_led_write(sr_state);
}

// ACTIVE-LOW bit control
static inline void sr_set_bit(uint8_t bit, bool on) {
    if (on) sr_state &= ~(1u << bit);   // ON  -> 0
    else    sr_state |=  (1u << bit);   // OFF -> 1
    sr_commit();
}

static inline void segments_all_off(void) {
    // ACTIVE-LOW: HIGH = OFF
    writePinHigh(GP5); // seg 1 OFF
    writePinHigh(GP4); // seg 2 OFF
    sr_state = 0xFF;   // SR outputs OFF
    sr_commit();
}

static inline void segment_set(uint8_t seg, bool on) {
    switch (seg) {
        case 1:
            if (on) writePinLow(GP5); else writePinHigh(GP5);
            break;
        case 2:
            if (on) writePinLow(GP4); else writePinHigh(GP4);
            break;

        // seg 3..10 => QA..QH => bits 0..7 (ACTIVE-LOW)
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

typedef enum {
    MODE_TEST = 0,
} display_mode_t;

static display_mode_t g_mode = MODE_TEST;

/* ─────────────────────────────────────────────
 * Init once at boot
 * ───────────────────────────────────────────── */
void keyboard_pre_init_user(void) {
    // Force test mode at startup for now
    g_mode = MODE_TEST;

    // Shift register pins
    setPinOutput(BAR_SER_PIN);
    setPinOutput(BAR_SRCLK_PIN);
    setPinOutput(BAR_RCLK_PIN);

    // Direct segment pins (active-low)
    setPinOutput(GP4); // seg 2
    setPinOutput(GP5); // seg 1

    // Known startup state: all OFF
    segments_all_off();
}

/* ─────────────────────────────────────────────
 * ALWAYS RUNS
 * Mode: TEST
 * Flash each segment one at a time (1..10), ~3x faster, loop
 * ───────────────────────────────────────────── */
void housekeeping_task_user(void) {
    static uint32_t last_step = 0;
    static uint8_t seg = 1;

    // 3x faster than 1000ms -> ~333ms
    if (timer_elapsed(last_step) < 333) {
        return;
    }
    last_step = timer_read();

    switch (g_mode) {
        case MODE_TEST:
        default:
            segments_all_off();        // all OFF (HIGH / 1)
            segment_set(seg, true);    // one ON  (LOW  / 0)

            seg++;
            if (seg > 10) {
                seg = 1;
            }
            break;
    }
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
                                                                   _______),

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
