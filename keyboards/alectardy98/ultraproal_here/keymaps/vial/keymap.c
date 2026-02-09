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
 * Init once at boot
 * ───────────────────────────────────────────── */
void keyboard_pre_init_user(void) {
    // Shift register pins
    setPinOutput(BAR_SER_PIN);
    setPinOutput(BAR_SRCLK_PIN);
    setPinOutput(BAR_RCLK_PIN);

    // GP4 / GP5
    setPinOutput(GP4);
    setPinOutput(GP5);

    // Known startup state: everything LOW
    writePinLow(GP4);
    writePinLow(GP5);
    bar_led_write(0x00);
}

/* ─────────────────────────────────────────────
 * ALWAYS RUNS
 * Toggle shift registers + GP4/GP5 together every 1 second
 * ───────────────────────────────────────────── */
void housekeeping_task_user(void) {
    static bool started = false;
    static uint32_t last_toggle;
    static bool state = false;

    if (!started) {
        started = true;
        last_toggle = timer_read();
        state = false;

        writePinLow(GP4);
        writePinLow(GP5);
        bar_led_write(0x00);
    }

    if (timer_elapsed(last_toggle) >= 1000) {
        last_toggle = timer_read();
        state = !state;

        if (state) {
            writePinHigh(GP4);
            writePinHigh(GP5);
            bar_led_write(0xFF);
        } else {
            writePinLow(GP4);
            writePinLow(GP5);
            bar_led_write(0x00);
        }
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
