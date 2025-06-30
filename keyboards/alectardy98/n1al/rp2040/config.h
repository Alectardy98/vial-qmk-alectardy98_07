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

#pragma once


// Data-in pin for WS2812
#define RGBLIGHT_PIN        GP0
#define WS2812_DRIVER = pwm
#define RGBLIGHT_LED_COUNT  4
#define RGBLIGHT_STARTUP_HUE    128
#define RGBLIGHT_STARTUP_SAT    255
#define RGBLIGHT_STARTUP_VAL    128
#define RGBLIGHT_LIMIT_VAL  255

// WS2812 Lighting Effect

#define RGBLIGHT_EFFECT_ALTERNATING
#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_CHRISTMAS
#define RGBLIGHT_EFFECT_KNIGHT
#define RGBLIGHT_EFFECT_RAINBOW_MOOD
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#define RGBLIGHT_EFFECT_RGB_TEST
#define RGBLIGHT_EFFECT_SNAKE
#define RGBLIGHT_EFFECT_STATIC_GRADIENT
#define RGBLIGHT_EFFECT_TWINKLE


#define RGB_MATRIX_DISABLE_SHARED_KEYCODES



/* Audio */
#define AUDIO_PIN GP1
#define AUDIO_PWM_DRIVER PWMD0
#define AUDIO_PWM_CHANNEL RP2040_PWM_CHANNEL_B
#define AUDIO_INIT_DELAY
#define AUDIO_CLICKY
#define AUDIO_VOICES
#define MUSIC_MAP

