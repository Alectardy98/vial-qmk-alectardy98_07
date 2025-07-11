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


// ── 74HC595 bar-graph ──────────────────────────────
// SER   → GP3
// SRCLK → GP1
// RCLK  → GP2
// (OE/RCLR tied to GND on the PCB)
// plus your two “extra” outputs for bars 1 & 2:
// BAR1 → GP4
// BAR2 → GP5

#define BAR_SR_SER_PIN    GP3
#define BAR_SR_SRCLK_PIN  GP1
#define BAR_SR_RCLK_PIN   GP2
#define BAR1_PIN          GP4
#define BAR2_PIN          GP5

// ── MAX7219 8-digit 7-seg ─────────────────────────
// LOAD → GP6   (CS)
// DIN  → GP7   (MOSI)
// CLK  → GP8   (SCK)

#define MAX7219_LOAD_PIN  GP6
#define MAX7219_DATA_PIN  GP7
#define MAX7219_CLK_PIN   GP8
#define MAX7219_NUM_DIGITS 8   // you wired DIG0…DIG7
