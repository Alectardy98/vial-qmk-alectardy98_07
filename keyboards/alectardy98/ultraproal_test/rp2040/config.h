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

/* Audio */
#define AUDIO_PIN GP0
#define AUDIO_PWM_DRIVER PWMD0
#define AUDIO_PWM_CHANNEL RP2040_PWM_CHANNEL_A
#define AUDIO_INIT_DELAY
#define AUDIO_CLICKY
#define AUDIO_VOICES
#define MUSIC_MAP

//
// 74HC595 bar‐graph shift register pins:
//   – SER = serial data in
//   – SRCLK = shift clock
//   – RCLK = latch clock (sometimes called “STCP”)
//
#define HC595_SER_PIN    GP3
#define HC595_SRCLK_PIN  GP4
#define HC595_RCLK_PIN   GP5

//
// MAX7219 8-digit display driver pins:
//   – CLK  = clock
//   – DIN  = serial data in
//   – LOAD = chip‐select / latch
//
#define MAX7219_CLK_PIN  GP6
#define MAX7219_DIN_PIN  GP7
#define MAX7219_LOAD_PIN GP8
