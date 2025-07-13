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


// —— 74HC595 pins ——
#define BAR_SER_PIN    GP3
#define BAR_SRCLK_PIN  GP1
#define BAR_RCLK_PIN   GP2


#define SPI_DRIVER        SPID0
#define MAX7219_DIN_PIN   GP7   // Data in (DIN)
#define MAX7219_CLK_PIN   GP6   // Clock (CLK)
#define MAX7219_CS_PIN    GP8   // Chip select / LOAD (CS)
