#include QMK_KEYBOARD_H

#ifdef RGB_MATRIX_ENABLE
#    include "color.h"    // hsv_to_rgb()
#    include "eeconfig.h"
#    include "timer.h"
#    ifdef __AVR__
#        include <avr/pgmspace.h>
#    endif
#endif

enum _layer {
    _BASE,
    _FN
};

// ---------------- Minimal keymaps (placeholders) ----------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
         KC_ESC, _______,   KC_F1,   KC_F2, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,      KC_INS, KC_HOME, KC_PGUP, _______,
        _______, _______,   KC_F3,   KC_F4,  KC_GRV,    KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0, KC_MINS,  KC_EQL, _______, KC_BSPC,      KC_DEL,  KC_END, KC_PGDN, _______,
        _______, _______,   KC_F5,   KC_F6,  KC_TAB,    KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P, KC_LBRC, KC_RBRC, KC_BSLS,                         KC_UP,
        _______, _______,   KC_F7,   KC_F8, KC_CAPS,    KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L, KC_SCLN, KC_QUOT,  KC_ENT,  KC_ENT,              KC_LEFT, KC_DOWN, KC_RGHT,
        _______, _______,   KC_F9,  KC_F10, KC_LSFT,    KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M, KC_COMM,  KC_DOT, KC_SLSH, KC_RSFT, MO(_FN),                                KC_DOWN,
        _______, _______,  KC_F11,  KC_F12,          MO(_FN),                                     KC_SPC,                                      KC_RALT,                                _______, _______, _______
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

#ifdef RGB_MATRIX_ENABLE

// ---------------- Custom Display Mode ----------------

#ifndef SECTION_MODE_MAX_VAL
#    define SECTION_MODE_MAX_VAL 100
#endif

// ---- Display groups (0–29) ----
static const uint8_t PROGMEM LEDS_QB_LOGO[]    = { 0, 1, 2 };
static const uint8_t PROGMEM LEDS_SCREEN[]     = { 3, 4, 5 };
static const uint8_t PROGMEM LEDS_CH_A[]       = { 6, 7, 8, 9, 10, 11, 12, 13 };
static const uint8_t PROGMEM LEDS_CH_B[]       = { 14, 15, 16, 17, 18, 19, 20, 21 };
static const uint8_t PROGMEM LEDS_AB_MIX[]     = { 22, 23, 24 };
static const uint8_t PROGMEM LEDS_AB_PREVIEW[] = { 25, 26, 27, 28, 29 };

// ---- Number sequences (one LED at a time), order = 5,3,1,2,4,6 ----
// KBD: 30=5,31=3,32=1,33=2,34=4,35=6
static const uint8_t PROGMEM SEQ_KBD[6] = { 30, 31, 32, 33, 34, 35 };
// A:   36=5,37=3,38=1,39=2,40=4,41=6
static const uint8_t PROGMEM SEQ_A[6]   = { 36, 37, 38, 39, 40, 41 };
// B:   42=5,43=3,44=1,45=2,46=4,47=6
static const uint8_t PROGMEM SEQ_B[6]   = { 42, 43, 44, 45, 46, 47 };

// --- Section bit positions (ONLY the 6 display toggles) ---
enum section_bits {
    BIT_QB_LOGO = 0,
    BIT_SCREEN,
    BIT_CH_A,
    BIT_CH_B,
    BIT_AB_MIX,
    BIT_AB_PREVIEW,
};

// Your preset:
// - Logo ON (rainbow)
// - Screen ON (red) + cycling KBD numbers (white)
// - Ch A OFF
// - Ch B OFF
// - AB Mix ON (red)
// - AB Preview ON (red)
#define PRESET_SECTION_MASK ((1u << BIT_QB_LOGO) | (1u << BIT_SCREEN) | (1u << BIT_AB_MIX) | (1u << BIT_AB_PREVIEW))

// ---- Color palette index (so we can persist cheaply) ----
enum disp_color_idx {
    DISP_WHT = 0,
    DISP_GRY,
    DISP_RED,
    DISP_MAG,
    DISP_BLU,
    DISP_CYN,
    DISP_GRN,
    DISP_YLW,
};

static bool    section_mode = true;     // start in custom mode
static uint8_t section_mask = 0;        // 6 bits
static uint8_t disp_idx     = DISP_RED; // default red for non-logo display LEDs

// Derived RGB for the chosen display color
static uint8_t disp_r = 255, disp_g = 0, disp_b = 0;

// Save/restore Vial/native RGB state when toggling modes
static bool    saved_enabled = false;
static uint8_t saved_mode    = 0;
static uint8_t saved_hue     = 0;
static uint8_t saved_sat     = 0;
static uint8_t saved_val     = 0;

// ---- Number animation state ----
#ifndef NUM_STEP_MS
#    define NUM_STEP_MS 250
#endif
static uint32_t num_timer = 0;
static uint8_t  num_step  = 0;

// ---- Persistent config in eeconfig_user() ----
// We store:
//  - signature (top byte)
//  - section_mask (bits 0..5)
//  - disp_idx     (bits 6..8)
#define USERCFG_SIG       0xA5u
#define USERCFG_SIG_SHIFT 24u

static inline void apply_disp_idx(uint8_t idx) {
    disp_idx = idx & 0x07;

    switch (disp_idx) {
        case DISP_WHT: disp_r = 255; disp_g = 255; disp_b = 255; break;
        case DISP_GRY: disp_r = 128; disp_g = 128; disp_b = 128; break;
        case DISP_RED: disp_r = 255; disp_g =   0; disp_b =   0; break;
        case DISP_MAG: disp_r = 255; disp_g =   0; disp_b = 255; break;
        case DISP_BLU: disp_r =   0; disp_g =   0; disp_b = 255; break;
        case DISP_CYN: disp_r =   0; disp_g = 255; disp_b = 255; break;
        case DISP_GRN: disp_r =   0; disp_g = 255; disp_b =   0; break;
        case DISP_YLW: disp_r = 255; disp_g = 255; disp_b =   0; break;
        default:       disp_r = 255; disp_g =   0; disp_b =   0; break;
    }
}

static inline uint32_t pack_user_cfg(void) {
    // IMPORTANT: cast to uint32_t and shift only 24 (never >= 32)
    uint32_t v = ((uint32_t)USERCFG_SIG << USERCFG_SIG_SHIFT);
    v |= (uint32_t)(section_mask & 0x3F);
    v |= ((uint32_t)(disp_idx & 0x07)) << 6;
    return v;
}

static inline bool unpack_user_cfg(uint32_t v) {
    uint8_t sig = (uint8_t)(v >> USERCFG_SIG_SHIFT);
    if (sig != (uint8_t)USERCFG_SIG) return false;

    section_mask = (uint8_t)(v & 0x3F);
    apply_disp_idx((uint8_t)((v >> 6) & 0x07));
    return true;
}

static inline void save_user_cfg(void) {
    eeconfig_update_user(pack_user_cfg());
}

static inline void apply_preset_and_save(void) {
    section_mask = (uint8_t)PRESET_SECTION_MASK;
    apply_disp_idx(DISP_RED);
    save_user_cfg();

    // reset animation cleanly whenever we jump back to preset
    num_timer = timer_read32();
    num_step  = 0;
}

static inline void enter_custom_mode(void) {
    // capture whatever Vial/native is doing right now
    saved_enabled = rgb_matrix_is_enabled();
    saved_mode    = rgb_matrix_get_mode();
    saved_hue     = rgb_matrix_get_hue();
    saved_sat     = rgb_matrix_get_sat();
    saved_val     = rgb_matrix_get_val();

    rgb_matrix_enable_noeeprom();
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);

    uint8_t v = saved_val;
    if (v > SECTION_MODE_MAX_VAL) v = SECTION_MODE_MAX_VAL;
    rgb_matrix_sethsv_noeeprom(saved_hue, saved_sat, v);

    num_timer = timer_read32();
    num_step  = 0;
}

static inline void exit_custom_mode(void) {
    // restore Vial/native state
    if (!saved_enabled) {
        rgb_matrix_disable_noeeprom();
        return;
    }
    rgb_matrix_mode_noeeprom(saved_mode);
    rgb_matrix_sethsv_noeeprom(saved_hue, saved_sat, saved_val);
}

void keyboard_post_init_user(void) {
    // If config is missing/invalid, force your preset.
    uint32_t u = eeconfig_read_user();
    if (!unpack_user_cfg(u)) {
        apply_preset_and_save();
    }

    // Always start in custom mode
    section_mode = true;
    enter_custom_mode();
}

// Keyboard-specific custom keycodes (QK_KB_0 style)
enum custom_keycodes {
    KB_SEC_MODE = QK_KB_0,
    KB_CLR_SECS,

    // 6 display toggles
    KB_TOG_LOGO,
    KB_TOG_SCREEN,
    KB_TOG_CH_A,
    KB_TOG_CH_B,
    KB_TOG_AB_MIX,
    KB_TOG_AB_PREV,

    // Display color keycodes (affect non-logo display LEDs 0–29)
    KB_DISP_WHT,
    KB_DISP_GRY,
    KB_DISP_RED,
    KB_DISP_MAG,
    KB_DISP_BLU,
    KB_DISP_CYN,
    KB_DISP_GRN,
    KB_DISP_YLW,
};

static inline void toggle_section_bit(uint8_t bit) {
    section_mask ^= (1u << bit);
}
static inline bool bit_enabled(uint8_t bit) {
    return (section_mask & (1u << bit)) != 0;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    switch (keycode) {
        case KB_SEC_MODE:
            section_mode = !section_mode;
            if (section_mode) enter_custom_mode();
            else              exit_custom_mode();
            return false;

        case KB_CLR_SECS:
            // Reset to YOUR preset (not "all off")
            apply_preset_and_save();
            return false;

        case KB_TOG_LOGO:    toggle_section_bit(BIT_QB_LOGO);    save_user_cfg(); return false;
        case KB_TOG_SCREEN:  toggle_section_bit(BIT_SCREEN);     save_user_cfg(); return false;
        case KB_TOG_CH_A:    toggle_section_bit(BIT_CH_A);       save_user_cfg(); return false;
        case KB_TOG_CH_B:    toggle_section_bit(BIT_CH_B);       save_user_cfg(); return false;
        case KB_TOG_AB_MIX:  toggle_section_bit(BIT_AB_MIX);     save_user_cfg(); return false;
        case KB_TOG_AB_PREV: toggle_section_bit(BIT_AB_PREVIEW); save_user_cfg(); return false;

        case KB_DISP_WHT: apply_disp_idx(DISP_WHT); save_user_cfg(); return false;
        case KB_DISP_GRY: apply_disp_idx(DISP_GRY); save_user_cfg(); return false;
        case KB_DISP_RED: apply_disp_idx(DISP_RED); save_user_cfg(); return false;
        case KB_DISP_MAG: apply_disp_idx(DISP_MAG); save_user_cfg(); return false;
        case KB_DISP_BLU: apply_disp_idx(DISP_BLU); save_user_cfg(); return false;
        case KB_DISP_CYN: apply_disp_idx(DISP_CYN); save_user_cfg(); return false;
        case KB_DISP_GRN: apply_disp_idx(DISP_GRN); save_user_cfg(); return false;
        case KB_DISP_YLW: apply_disp_idx(DISP_YLW); save_user_cfg(); return false;
    }
    return true;
}

// ---- Small helpers (cheap on AVR) ----
static inline uint8_t scale8(uint8_t c, uint8_t v) {
    return (uint16_t)c * v / 255;
}

static void paint_pgm_section_scaled(const uint8_t *arr_pgm, uint8_t len,
                                     uint8_t led_min, uint8_t led_max,
                                     uint8_t r, uint8_t g, uint8_t b) {
    for (uint8_t i = 0; i < len; i++) {
#ifdef __AVR__
        uint8_t idx = pgm_read_byte(&arr_pgm[i]);
#else
        uint8_t idx = arr_pgm[i];
#endif
        if (idx >= RGB_MATRIX_LED_COUNT) continue;
        if (idx >= led_min && idx < led_max) {
            rgb_matrix_set_color(idx, r, g, b);
        }
    }
}

static void paint_single_from_seq(const uint8_t *seq_pgm, uint8_t step,
                                  uint8_t led_min, uint8_t led_max,
                                  uint8_t r, uint8_t g, uint8_t b) {
#ifdef __AVR__
    uint8_t idx = pgm_read_byte(&seq_pgm[step]);
#else
    uint8_t idx = seq_pgm[step];
#endif
    if (idx >= RGB_MATRIX_LED_COUNT) return;
    if (idx >= led_min && idx < led_max) {
        rgb_matrix_set_color(idx, r, g, b);
    }
}

// Rainbow only for the logo (3 LEDs)
static void paint_logo_rainbow(uint8_t led_min, uint8_t led_max, uint8_t v_cap) {
    // Hue cycles over time
    uint8_t base_h = (uint8_t)((timer_read32() / 12) & 0xFF);

    for (uint8_t i = 0; i < (uint8_t)sizeof(LEDS_QB_LOGO); i++) {
#ifdef __AVR__
        uint8_t idx = pgm_read_byte(&LEDS_QB_LOGO[i]);
#else
        uint8_t idx = LEDS_QB_LOGO[i];
#endif
        if (idx >= RGB_MATRIX_LED_COUNT) continue;
        if (idx < led_min || idx >= led_max) continue;

        HSV hsv = { (uint8_t)(base_h + (i * 32)), 255, v_cap };
        RGB rgb = hsv_to_rgb(hsv);
        rgb_matrix_set_color(idx, rgb.r, rgb.g, rgb.b);
    }
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (!section_mode) return true;

    // Clear ONLY this batch
    for (uint8_t i = led_min; i < led_max; i++) {
        rgb_matrix_set_color(i, 0, 0, 0);
    }

    // Update number animation step
    if (timer_elapsed32(num_timer) >= NUM_STEP_MS) {
        num_timer = timer_read32();
        num_step++;
        if (num_step >= 6) num_step = 0;
    }

    // Brightness cap (brownout protection)
    uint8_t v = rgb_matrix_get_val();
    if (v > SECTION_MODE_MAX_VAL) v = SECTION_MODE_MAX_VAL;

    // Scaled “display color” (for non-logo LEDs 0–29)
    const uint8_t d_r = scale8(disp_r, v);
    const uint8_t d_g = scale8(disp_g, v);
    const uint8_t d_b = scale8(disp_b, v);

    // ALL numbers white
    const uint8_t n_r = scale8(255, v);
    const uint8_t n_g = scale8(255, v);
    const uint8_t n_b = scale8(255, v);

    // ---- Display groups (0–29) ----
    // Logo: rainbow when enabled
    if (bit_enabled(BIT_QB_LOGO)) {
        paint_logo_rainbow(led_min, led_max, v);
    }

    // Everything else uses chosen display color (default red)
    if (bit_enabled(BIT_SCREEN))     paint_pgm_section_scaled(LEDS_SCREEN,     sizeof(LEDS_SCREEN),     led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_CH_A))       paint_pgm_section_scaled(LEDS_CH_A,       sizeof(LEDS_CH_A),       led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_CH_B))       paint_pgm_section_scaled(LEDS_CH_B,       sizeof(LEDS_CH_B),       led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_AB_MIX))     paint_pgm_section_scaled(LEDS_AB_MIX,     sizeof(LEDS_AB_MIX),     led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_AB_PREVIEW)) paint_pgm_section_scaled(LEDS_AB_PREVIEW, sizeof(LEDS_AB_PREVIEW), led_min, led_max, d_r, d_g, d_b);

    // ---- Numbers: automatic “one-at-a-time” sequences ----
    if (bit_enabled(BIT_SCREEN)) paint_single_from_seq(SEQ_KBD, num_step, led_min, led_max, n_r, n_g, n_b);
    if (bit_enabled(BIT_CH_A))   paint_single_from_seq(SEQ_A,   num_step, led_min, led_max, n_r, n_g, n_b);
    if (bit_enabled(BIT_CH_B))   paint_single_from_seq(SEQ_B,   num_step, led_min, led_max, n_r, n_g, n_b);

    return false;
}

#endif // RGB_MATRIX_ENABLE
