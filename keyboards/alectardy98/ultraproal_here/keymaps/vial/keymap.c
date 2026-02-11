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

// Use QMK's audio engine (PWM on RP2040) instead of manually toggling GPIO
#include "audio.h"

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
 * Custom keycodes (Vial-friendly)
 * ───────────────────────────────────────────── */
enum custom_keycode {
    LEDT = QK_KB_0, // toggle DEFAULT <-> TEST
    CWMD,           // toggle DEFAULT <-> CW
    WPMT,           // enter/commit WPM edit (CW mode only)
};

/* ─────────────────────────────────────────────
 * Display modes
 * ───────────────────────────────────────────── */
typedef enum {
    MODE_DEFAULT = 0, // BOTH displays OFF
    MODE_TEST    = 1, // Bar + MAX7219 test patterns
    MODE_CW      = 2, // CW mode: WPM display + audio sidetone
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
        case 1:  if (on) writePinLow(GP5); else writePinHigh(GP5); break;
        case 2:  if (on) writePinLow(GP4); else writePinHigh(GP4); break;
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
 * CW Audio + WPM + Morse scheduler
 * ───────────────────────────────────────────── */

static uint16_t cw_wpm = 20;

// WPM entry state (CW mode only)
static bool     cw_edit_active = false;
static char     cw_edit_buf[4] = {0}; // up to "99" + NUL
static uint8_t  cw_edit_len    = 0;

static inline uint16_t cw_dit_ms(void) {
    if (cw_wpm < 1) return 1200;
    return (uint16_t)(1200u / cw_wpm);
}

// Dot weighting to avoid "too-short dot" glitches at high WPM
#ifndef CW_DOT_EXTRA_MS
#    define CW_DOT_EXTRA_MS 12u  // was 8u; make dots a bit longer
#endif

#ifndef CW_DOT_MIN_MS
#    define CW_DOT_MIN_MS   30u  // was 28u; slightly longer minimum dot
#endif

// IMPORTANT: In QMK, KC_0 is NOT between KC_1 and KC_9.
static inline bool is_digit_kc(uint16_t kc) {
    return (kc == KC_0) || (kc >= KC_1 && kc <= KC_9);
}
static inline char digit_from_kc(uint16_t kc) {
    return (kc == KC_0) ? '0' : (char)('0' + (kc - KC_1 + 1));
}

/* ─────────────────────────────────────────────
 * CW sidetone using QMK Audio (PWM)
 * ───────────────────────────────────────────── */
static float cw_tone_hz = 600.0f;
static bool  cw_tone_running = false;

static inline void cw_tone_engine_start(void) {
    if (!is_audio_on()) return;
    if (cw_tone_running) return;
    cw_tone_running = true;
    play_note(cw_tone_hz, 1);
}

static inline void cw_tone_engine_stop(void) {
    cw_tone_running = false;
    stop_all_notes();
}

static inline void cw_tone_on(void)  { cw_tone_engine_start(); }
static inline void cw_tone_off(void) { cw_tone_engine_stop();  }

static inline void cw_audio_hard_stop(void) { cw_tone_engine_stop(); }

/* ─────────────────────────────────────────────
 * Morse table
 * ───────────────────────────────────────────── */
static const char *cw_morse_for(char c) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');

    switch (c) {
        case 'A': return ".-";
        case 'B': return "-...";
        case 'C': return "-.-.";
        case 'D': return "-..";
        case 'E': return ".";
        case 'F': return "..-.";
        case 'G': return "--.";
        case 'H': return "....";
        case 'I': return "..";
        case 'J': return ".---";
        case 'K': return "-.-";
        case 'L': return ".-..";
        case 'M': return "--";
        case 'N': return "-.";
        case 'O': return "---";
        case 'P': return ".--.";
        case 'Q': return "--.-";
        case 'R': return ".-.";
        case 'S': return "...";
        case 'T': return "-";
        case 'U': return "..-";
        case 'V': return "...-";
        case 'W': return ".--";
        case 'X': return "-..-";
        case 'Y': return "-.--";
        case 'Z': return "--..";

        case '0': return "-----";
        case '1': return ".----";
        case '2': return "..---";
        case '3': return "...--";
        case '4': return "....-";
        case '5': return ".....";
        case '6': return "-....";
        case '7': return "--...";
        case '8': return "---..";
        case '9': return "----.";

        default:  return NULL;
    }
}

/* ─────────────────────────────────────────────
 * Display WPM on MAX7219 digits 2..3 (00–99), blank others
 * ───────────────────────────────────────────── */
static void cw_display_wpm(uint16_t wpm) {
    if (wpm > 99) wpm = 99;

    uint8_t tens = (uint8_t)(wpm / 10u);
    uint8_t ones = (uint8_t)(wpm % 10u);

    // Blank 0..1
    max7219_write_digit(0, 0x0F);
    max7219_write_digit(1, 0x0F);

    // Show WPM on 2..3
    if (MAX7219_NUM_DIGITS > 2) max7219_write_digit(2, tens);
    if (MAX7219_NUM_DIGITS > 3) max7219_write_digit(3, ones);
}

/* ─────────────────────────────────────────────
 * Type-ahead queue (~500 chars) + time-remaining bargraph
 * ───────────────────────────────────────────── */

typedef enum {
    CW_IDLE = 0,
    CW_TONE_ON,
    CW_GAP,
} cw_play_state_t;

#define CW_QSIZE 512u
#define CW_QMASK (CW_QSIZE - 1u)

static char     cw_q[CW_QSIZE];
static uint16_t cw_q_r = 0, cw_q_w = 0;
static uint32_t cw_queue_dits_total = 0;

static inline uint16_t cw_q_len(void) { return (uint16_t)((cw_q_w - cw_q_r) & CW_QMASK); }
static inline bool cw_q_empty(void)   { return cw_q_r == cw_q_w; }
static inline bool cw_q_full(void)    { return cw_q_len() >= (CW_QSIZE - 1u); }

#ifndef CW_BAR_FULL_MS
#    define CW_BAR_FULL_MS 30000u
#endif

static inline uint16_t cw_symbol_dits(char sym) { return (sym == '-') ? 3u : 1u; }

static uint16_t cw_char_dits(char c) {
    if (c == ' ') return 7u;

    const char *p = cw_morse_for(c);
    if (!p) return 3u;

    uint16_t dits = 0;
    uint16_t n = 0;
    while (*p) {
        dits += cw_symbol_dits(*p);
        n++;
        p++;
    }
    if (n > 1) dits += (uint16_t)(n - 1u);
    dits += 3u;
    return dits;
}

static inline void cw_q_push(char c) {
    if (cw_q_full()) return;
    cw_q[cw_q_w] = c;
    cw_q_w = (cw_q_w + 1u) & CW_QMASK;
    cw_queue_dits_total += cw_char_dits(c);
}

static inline char cw_q_pop(void) {
    char c = cw_q[cw_q_r];
    cw_q_r = (cw_q_r + 1u) & CW_QMASK;
    return c;
}

static inline bool cw_q_pop_last(void) {
    if (cw_q_empty()) return false;
    uint16_t last = (cw_q_w - 1u) & CW_QMASK;
    char c = cw_q[last];
    cw_q_w = last;

    uint16_t d = cw_char_dits(c);
    cw_queue_dits_total = (cw_queue_dits_total >= d) ? (cw_queue_dits_total - d) : 0;
    return true;
}

static inline uint32_t cw_estimate_queue_ms(void) {
    return cw_queue_dits_total * (uint32_t)cw_dit_ms();
}

/* ─────────────────────────────────────────────
 * Queue overflow lockout + error feedback (CW mode only)
 * ───────────────────────────────────────────── */
static bool     cw_queue_locked = false;
static uint32_t cw_err_flash_until = 0;

#ifndef CW_ERR_FLASH_MS
#    define CW_ERR_FLASH_MS 200u
#endif

static inline void cw_error_beep_and_flash(void) {
    cw_queue_locked = true;
    cw_err_flash_until = timer_read32() + CW_ERR_FLASH_MS;

    if (!is_audio_on()) return;
    play_note(200.0f, 1);
    wait_ms(60);
    stop_all_notes();
}

/* ─────────────────────────────────────────────
 * Bargraph update throttling
 * ───────────────────────────────────────────── */
#ifndef CW_BAR_UPDATE_MS
#    define CW_BAR_UPDATE_MS 40u
#endif

static uint32_t cw_bar_last_update = 0;
static uint8_t  cw_bar_cached_level = 0;

static uint8_t cw_bar_level_from_ms(uint32_t ms) {
    uint8_t level;
    if (ms == 0) level = 0;
    else if (ms >= (uint32_t)CW_BAR_FULL_MS) level = 10;
    else {
        uint32_t bumped = ms + (CW_BAR_FULL_MS / 20u);
        level = (uint8_t)((bumped * 10u) / (uint32_t)CW_BAR_FULL_MS);
        if (level > 10) level = 10;
    }
    return level;
}

static void cw_bar_apply_level(uint8_t level, bool force) {
    if (!force && level == cw_bar_cached_level) return;
    cw_bar_cached_level = level;

    segments_all_off();
    for (uint8_t s = 1; s <= level; s++) segment_set(s, true);
}

static void cw_bar_update_time_remaining(void) {
    uint32_t now = timer_read32();

    bool flash = (cw_err_flash_until != 0) && ((int32_t)(now - cw_err_flash_until) < 0);
    bool force = flash;

    if (!force && TIMER_DIFF_32(now, cw_bar_last_update) < CW_BAR_UPDATE_MS) return;
    cw_bar_last_update = now;

    uint8_t level = cw_bar_level_from_ms(cw_estimate_queue_ms());
    cw_bar_apply_level(level, force);

    if (flash) {
        segment_set(10, true);
    } else if (cw_err_flash_until != 0 && (int32_t)(now - cw_err_flash_until) >= 0) {
        cw_err_flash_until = 0;
    }
}

/* ─────────────────────────────────────────────
 * Playback working state
 * ───────────────────────────────────────────── */
static cw_play_state_t cw_state = CW_IDLE;
static uint32_t cw_next_event_ms = 0;

static const char *cw_pat = NULL;
static uint8_t cw_pat_idx = 0;
static bool cw_intra_symbol_gap = false;

static void cw_start_next_char(void) {
    if (cw_q_empty()) {
        cw_state = CW_IDLE;
        cw_tone_off();
        return;
    }

    cw_queue_locked = false;

    char c = cw_q_pop();

    uint16_t d = cw_char_dits(c);
    cw_queue_dits_total = (cw_queue_dits_total >= d) ? (cw_queue_dits_total - d) : 0;

    if (c == ' ') {
        cw_state = CW_GAP;
        cw_tone_off();
        cw_next_event_ms = timer_read32() + (uint32_t)(7u * cw_dit_ms());
        return;
    }

    cw_pat = cw_morse_for(c);
    if (!cw_pat) {
        cw_state = CW_GAP;
        cw_tone_off();
        cw_next_event_ms = timer_read32() + (uint32_t)(3u * cw_dit_ms());
        return;
    }

    cw_pat_idx = 0;
    cw_intra_symbol_gap = false;

    cw_state = CW_TONE_ON;
    cw_tone_on();

    uint16_t dit = cw_dit_ms();
    uint16_t dur;

    if (cw_pat[cw_pat_idx] == '-') {
        dur = (uint16_t)(3u * dit);
    } else {
        uint16_t dot = (uint16_t)(dit + CW_DOT_EXTRA_MS);
        if (dot < CW_DOT_MIN_MS) dot = CW_DOT_MIN_MS;
        dur = dot;
    }

    cw_next_event_ms = timer_read32() + dur;
}

static void cw_task(void) {
    if (g_mode != MODE_CW) {
        cw_state = CW_IDLE;
        cw_audio_hard_stop();
        return;
    }

    if (!is_audio_on() && cw_tone_running) cw_audio_hard_stop();

    max7219_force_on();
    if (cw_edit_active) {
        uint16_t tmp = 0;
        for (uint8_t i = 0; i < cw_edit_len; i++) tmp = (uint16_t)(tmp * 10u + (uint16_t)(cw_edit_buf[i] - '0'));
        cw_display_wpm(tmp);
    } else {
        cw_display_wpm(cw_wpm);
    }

    uint32_t now = timer_read32();

    if (cw_state == CW_IDLE) {
        if (!cw_q_empty()) cw_start_next_char();
        return;
    }

    if ((int32_t)(now - cw_next_event_ms) < 0) return;

    uint16_t dit = cw_dit_ms();

    if (cw_state == CW_TONE_ON) {
        cw_tone_off();
        cw_state = CW_GAP;
        cw_next_event_ms = now + dit;
        cw_intra_symbol_gap = true;
        return;
    }

    if (cw_intra_symbol_gap && cw_pat) {
        cw_pat_idx++;
        cw_intra_symbol_gap = false;

        if (cw_pat[cw_pat_idx] != '\0') {
            cw_state = CW_TONE_ON;
            cw_tone_on();

            uint16_t dur;
            if (cw_pat[cw_pat_idx] == '-') {
                dur = (uint16_t)(3u * dit);
            } else {
                uint16_t dot = (uint16_t)(dit + CW_DOT_EXTRA_MS);
                if (dot < CW_DOT_MIN_MS) dot = CW_DOT_MIN_MS;
                dur = dot;
            }

            cw_next_event_ms = now + dur;
            return;
        }

        cw_pat = NULL;
        cw_state = CW_GAP;
        cw_next_event_ms = now + (uint32_t)(2u * dit);
        return;
    }

    cw_start_next_char();
}

/* ─────────────────────────────────────────────
 * Startup animation (no loop)
 * ───────────────────────────────────────────── */

#define START_BAR_STEP_MS   111u
#define START_BAR_STEPS     19u
#define START_TOTAL_MS      (START_BAR_STEP_MS * START_BAR_STEPS)

#define START_STAGE0_END    (START_TOTAL_MS / 4u)
#define START_STAGE1_END    (2u * (START_TOTAL_MS / 4u))
#define START_STAGE2_END    (3u * (START_TOTAL_MS / 4u))
#define START_STAGE3_END    (START_TOTAL_MS)

static bool     g_startup_running = false;
static uint32_t g_startup_t0      = 0;

/* ─────────────────────────────────────────────
 * Mode switching helper
 * ───────────────────────────────────────────── */
static inline void set_display_mode(display_mode_t mode) {
    g_mode = mode;

    if (g_mode == MODE_DEFAULT) {
        segments_all_off();
        max7219_blank_all();

        cw_edit_active = false;
        cw_edit_len = 0;
        cw_edit_buf[0] = '\0';

        cw_q_r = cw_q_w = 0;
        cw_queue_dits_total = 0;

        cw_state = CW_IDLE;
        cw_queue_locked = false;
        cw_err_flash_until = 0;
        cw_bar_last_update = 0;
        cw_bar_cached_level = 0;

        cw_audio_hard_stop();
    } else if (g_mode == MODE_CW) {
        segments_all_off();
        max7219_blank_all();
        max7219_force_on();

        cw_wpm = 20;
        cw_edit_active = false;
        cw_edit_len = 0;
        cw_edit_buf[0] = '\0';

        cw_q_r = cw_q_w = 0;
        cw_queue_dits_total = 0;

        cw_state = CW_IDLE;
        cw_queue_locked = false;
        cw_err_flash_until = 0;
        cw_bar_last_update = timer_read32();
        cw_bar_cached_level = 0;

        cw_audio_hard_stop();

        cw_display_wpm(cw_wpm);
    }
}

/* ─────────────────────────────────────────────
 * Init hooks
 * ───────────────────────────────────────────── */
void keyboard_pre_init_user(void) {
    setPinOutput(BAR_SER_PIN);
    setPinOutput(BAR_SRCLK_PIN);
    setPinOutput(BAR_RCLK_PIN);

    setPinOutput(GP4); // seg 2
    setPinOutput(GP5); // seg 1

    cw_audio_hard_stop();

    segments_all_off();
    g_mode = MODE_DEFAULT;
}

void keyboard_post_init_user(void) {
    setPinOutput(MAX7219_CS_PIN);
    writePinHigh(MAX7219_CS_PIN);

    max7219_init();

    g_startup_running = true;
    g_startup_t0 = timer_read32();

    segments_all_off();
    max7219_blank_all();
    max7219_force_on();

    g_mode = MODE_DEFAULT;
}

/* ─────────────────────────────────────────────
 * Startup animation tasks
 * ───────────────────────────────────────────── */
static void startup_bar_task(void) {
    uint32_t now = timer_read32();
    uint32_t elapsed = TIMER_DIFF_32(now, g_startup_t0);

    if (elapsed >= START_TOTAL_MS) {
        segments_all_off();
        return;
    }

    uint32_t step = elapsed / START_BAR_STEP_MS;
    if (step >= START_BAR_STEPS) step = START_BAR_STEPS - 1;

    uint8_t seg = (step <= 9) ? (uint8_t)(1u + step) : (uint8_t)(19u - step);

    segments_all_off();
    segment_set(seg, true);
}

static void startup_max_task(void) {
    uint32_t now = timer_read32();
    uint32_t elapsed = TIMER_DIFF_32(now, g_startup_t0);

    if (elapsed >= START_TOTAL_MS) {
        max7219_blank_all();
        return;
    }

    max7219_force_on();

    uint8_t stage = 0;
    if (elapsed < START_STAGE0_END) stage = 0;
    else if (elapsed < START_STAGE1_END) stage = 1;
    else if (elapsed < START_STAGE2_END) stage = 2;
    else stage = 3;

    uint8_t out[4] = {0x0F, 0x0F, 0x0F, 0x0F};
    out[0] = 3;
    if (stage >= 1) out[1] = 2;
    if (stage >= 2) out[2] = 1;
    if (stage >= 3) out[3] = 0;

    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS && d < 4; d++) max7219_write_digit(d, out[d]);
}

void housekeeping_task_user(void) {
    if (g_startup_running) {
        startup_bar_task();
        return;
    }

    if (g_mode == MODE_CW) {
        cw_bar_update_time_remaining();
        return;
    }

    if (g_mode != MODE_TEST) return;

    static uint32_t last_step = 0;
    static uint8_t seg = 1;
    static bool invert_pass = false;

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

void matrix_scan_user(void) {
    if (g_startup_running) {
        startup_max_task();

        uint32_t now = timer_read32();
        uint32_t elapsed = TIMER_DIFF_32(now, g_startup_t0);
        if (elapsed >= START_TOTAL_MS) {
            g_startup_running = false;
            segments_all_off();
            max7219_blank_all();
            set_display_mode(MODE_DEFAULT);
        }
        return;
    }

    if (g_mode == MODE_CW) {
        cw_task();
        return;
    }

    if (g_mode != MODE_TEST) return;

    #define MAX7219_STEP_MS 200

    static uint32_t last = 0;
    static uint8_t phase = 0;
    static uint8_t val = 0;

    uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, last) < MAX7219_STEP_MS) return;
    last = now;

    max7219_force_on();

    uint8_t out[4] = {0x0F, 0x0F, 0x0F, 0x0F};

    if (phase < 4) {
        if (phase < MAX7219_NUM_DIGITS) out[phase] = val;
    } else {
        for (uint8_t d = 0; d < MAX7219_NUM_DIGITS && d < 4; d++) out[d] = val;
    }

    for (uint8_t d = 0; d < MAX7219_NUM_DIGITS && d < 4; d++) max7219_write_digit(d, out[d]);

    val++;
    if (val > 9) {
        val = 0;
        phase++;
        if (phase > 4) phase = 0;
    }
}

/* ─────────────────────────────────────────────
 * Macro handler
 * ───────────────────────────────────────────── */
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    if (g_mode == MODE_CW && cw_edit_active) {
        if (keycode == WPMT) {
            // fall through
        } else if (is_digit_kc(keycode)) {
            if (cw_edit_len < 2) {
                cw_edit_buf[cw_edit_len++] = digit_from_kc(keycode);
                cw_edit_buf[cw_edit_len] = '\0';
            }
            return false;
        } else if (keycode == KC_BSPC) {
            if (cw_edit_len > 0) {
                cw_edit_len--;
                cw_edit_buf[cw_edit_len] = '\0';
            }
            return false;
        } else {
            return false;
        }
    }

    switch (keycode) {
        case LEDT:
            if (g_startup_running) return false;
            set_display_mode((g_mode == MODE_TEST) ? MODE_DEFAULT : MODE_TEST);
            return false;

        case CWMD:
            if (g_startup_running) return false;
            set_display_mode((g_mode == MODE_CW) ? MODE_DEFAULT : MODE_CW);
            return false;

        case WPMT:
            if (g_mode != MODE_CW) return false;

            if (!cw_edit_active) {
                cw_edit_active = true;
                cw_edit_len = 0;
                cw_edit_buf[0] = '\0';
            } else {
                uint16_t v = 0;
                for (uint8_t i = 0; i < cw_edit_len; i++) v = (uint16_t)(v * 10u + (uint16_t)(cw_edit_buf[i] - '0'));
                if (v < 1) v = 1;
                if (v > 99) v = 99;
                cw_wpm = v;

                cw_edit_active = false;
                cw_edit_len = 0;
                cw_edit_buf[0] = '\0';

                cw_display_wpm(cw_wpm);
            }
            return false;
    }

    if (g_mode == MODE_CW) {
        if (keycode == KC_BSPC) {
            cw_q_pop_last();
            return false;
        }

        if (cw_queue_locked) {
            if ((keycode >= KC_A && keycode <= KC_Z) || is_digit_kc(keycode) || keycode == KC_SPC) {
                cw_error_beep_and_flash();
                return false;
            }
            return true;
        }

        if ((keycode >= KC_A && keycode <= KC_Z) || is_digit_kc(keycode) || keycode == KC_SPC) {
            if (cw_q_full()) {
                cw_error_beep_and_flash();
                return false;
            }
        }

        if (keycode >= KC_A && keycode <= KC_Z) {
            cw_q_push((char)('A' + (keycode - KC_A)));
        } else if (is_digit_kc(keycode)) {
            cw_q_push(digit_from_kc(keycode));
        } else if (keycode == KC_SPC) {
            cw_q_push(' ');
        }

        return true;
    }

    return true;
}

/* ─────────────────────────────────────────────
 * Keymaps
 * ─────────────────────────────────────────────
 */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[_BASE] = LAYOUT(
    QK_BOOT,    KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC, KC_BSPC,   LEDT,
     KC_TAB,    _______, KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,   CWMD,
    _______, KC_CAPS,    KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,    _______,   WPMT,
    _______, KC_LSFT,    KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT, _______,   _______,
                                                           KC_SPC
),

[_FN] = LAYOUT(
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
    _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,
                                                             CWMD
),
};

const uint8_t music_map[MATRIX_ROWS][MATRIX_COLS] = LAYOUT(
    45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,  0,  0,  0,
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
                              0
);
