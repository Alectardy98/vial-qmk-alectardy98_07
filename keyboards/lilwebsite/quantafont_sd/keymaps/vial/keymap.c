#include QMK_KEYBOARD_H

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

// ---------------- RGB Matrix "Section Mode" (Vial-friendly, AVR-safe) ----------------

#ifdef __AVR__
#    include <avr/pgmspace.h>
#else
#    include <string.h>
#endif

#ifndef SECTION_MODE_MAX_VAL
#    define SECTION_MODE_MAX_VAL 100  // raise carefully; too high can cause USB brownout resets
#endif

// --- Sections (LED index lists in PROGMEM) ---
static const uint8_t PROGMEM LEDS_QB_LOGO[]    = { 0, 1, 2 };
static const uint8_t PROGMEM LEDS_SCREEN[]     = { 3, 4, 5 };
static const uint8_t PROGMEM LEDS_CH_A[]       = { 6, 7, 8, 9, 10, 11, 12, 13 };
static const uint8_t PROGMEM LEDS_CH_B[]       = { 14, 15, 16, 17, 18, 19, 20, 21 };
static const uint8_t PROGMEM LEDS_AB_MIX[]     = { 22, 23, 24 };
static const uint8_t PROGMEM LEDS_AB_PREVIEW[] = { 25, 26, 27, 28, 29 };

// Keyboard numbers: 30=5,31=3,32=1,33=2,34=4,35=6
static const uint8_t PROGMEM LED_KBD_1[] = { 32 };
static const uint8_t PROGMEM LED_KBD_2[] = { 33 };
static const uint8_t PROGMEM LED_KBD_3[] = { 31 };
static const uint8_t PROGMEM LED_KBD_4[] = { 34 };
static const uint8_t PROGMEM LED_KBD_5[] = { 30 };
static const uint8_t PROGMEM LED_KBD_6[] = { 35 };

// Channel A numbers: 36=5,37=3,38=1,39=2,40=4,41=6
static const uint8_t PROGMEM LED_A_1[] = { 38 };
static const uint8_t PROGMEM LED_A_2[] = { 39 };
static const uint8_t PROGMEM LED_A_3[] = { 37 };
static const uint8_t PROGMEM LED_A_4[] = { 40 };
static const uint8_t PROGMEM LED_A_5[] = { 36 };
static const uint8_t PROGMEM LED_A_6[] = { 41 };

// Channel B numbers: 42=5,43=3,44=1,45=2,46=4,47=6
static const uint8_t PROGMEM LED_B_1[] = { 44 };
static const uint8_t PROGMEM LED_B_2[] = { 45 };
static const uint8_t PROGMEM LED_B_3[] = { 43 };
static const uint8_t PROGMEM LED_B_4[] = { 46 };
static const uint8_t PROGMEM LED_B_5[] = { 42 };
static const uint8_t PROGMEM LED_B_6[] = { 47 };

// --- Section bit positions (one bit per toggle) ---
enum section_bits {
    BIT_QB_LOGO = 0,
    BIT_SCREEN,
    BIT_CH_A,
    BIT_CH_B,
    BIT_AB_MIX,
    BIT_AB_PREVIEW,

    BIT_KBD_1, BIT_KBD_2, BIT_KBD_3, BIT_KBD_4, BIT_KBD_5, BIT_KBD_6,
    BIT_A_1,   BIT_A_2,   BIT_A_3,   BIT_A_4,   BIT_A_5,   BIT_A_6,
    BIT_B_1,   BIT_B_2,   BIT_B_3,   BIT_B_4,   BIT_B_5,   BIT_B_6,
};

static bool     section_mode = false;
static uint32_t section_mask = 0;

// Save/restore user's RGB state when entering/exiting section mode
static uint8_t saved_mode = 0;
static uint8_t saved_hue  = 0;
static uint8_t saved_sat  = 0;
static uint8_t saved_val  = 0;

// One “display color” used for ALL display LEDs (0–29)
static uint8_t disp_r = 255;
static uint8_t disp_g = 0;
static uint8_t disp_b = 0;

static inline void set_display_color(uint8_t r, uint8_t g, uint8_t b) {
    disp_r = r;
    disp_g = g;
    disp_b = b;
}

static inline void toggle_section_bit(uint8_t bit) {
    section_mask ^= (1UL << bit);
}

// Keyboard-specific custom keycodes (QK_KB_0 style)
enum custom_keycodes {
    KB_SEC_MODE = QK_KB_0,  // toggle section mode on/off
    KB_CLR_SECS,            // clear all section toggles

    // 6 main groups
    KB_TOG_LOGO,
    KB_TOG_SCREEN,
    KB_TOG_CH_A,
    KB_TOG_CH_B,
    KB_TOG_AB_MIX,
    KB_TOG_AB_PREV,

    // keyboard 1-6
    KB_TOG_KBD_1, KB_TOG_KBD_2, KB_TOG_KBD_3, KB_TOG_KBD_4, KB_TOG_KBD_5, KB_TOG_KBD_6,

    // A 1-6
    KB_TOG_A_1, KB_TOG_A_2, KB_TOG_A_3, KB_TOG_A_4, KB_TOG_A_5, KB_TOG_A_6,

    // B 1-6
    KB_TOG_B_1, KB_TOG_B_2, KB_TOG_B_3, KB_TOG_B_4, KB_TOG_B_5, KB_TOG_B_6,

    // Display color keycodes (apply to LEDs 0–29 only)
    KB_DISP_WHT,
    KB_DISP_GRY,
    KB_DISP_RED,
    KB_DISP_MAG,
    KB_DISP_BLU,
    KB_DISP_CYN,
    KB_DISP_GRN,
    KB_DISP_YLW,
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) return true;

    switch (keycode) {
        case KB_SEC_MODE:
            section_mode = !section_mode;

            if (section_mode) {
                saved_mode = rgb_matrix_get_mode();
                saved_hue  = rgb_matrix_get_hue();
                saved_sat  = rgb_matrix_get_sat();
                saved_val  = rgb_matrix_get_val();

                rgb_matrix_enable_noeeprom();
                rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);

                uint8_t v = saved_val;
                if (v > SECTION_MODE_MAX_VAL) v = SECTION_MODE_MAX_VAL;
                rgb_matrix_sethsv_noeeprom(saved_hue, saved_sat, v);
            } else {
                rgb_matrix_mode_noeeprom(saved_mode);
                rgb_matrix_sethsv_noeeprom(saved_hue, saved_sat, saved_val);
            }
            return false;

        case KB_CLR_SECS:
            section_mask = 0;
            return false;

        // 6 groups
        case KB_TOG_LOGO:    toggle_section_bit(BIT_QB_LOGO);    return false;
        case KB_TOG_SCREEN:  toggle_section_bit(BIT_SCREEN);     return false;
        case KB_TOG_CH_A:    toggle_section_bit(BIT_CH_A);       return false;
        case KB_TOG_CH_B:    toggle_section_bit(BIT_CH_B);       return false;
        case KB_TOG_AB_MIX:  toggle_section_bit(BIT_AB_MIX);     return false;
        case KB_TOG_AB_PREV: toggle_section_bit(BIT_AB_PREVIEW); return false;

        // KBD 1-6
        case KB_TOG_KBD_1: toggle_section_bit(BIT_KBD_1); return false;
        case KB_TOG_KBD_2: toggle_section_bit(BIT_KBD_2); return false;
        case KB_TOG_KBD_3: toggle_section_bit(BIT_KBD_3); return false;
        case KB_TOG_KBD_4: toggle_section_bit(BIT_KBD_4); return false;
        case KB_TOG_KBD_5: toggle_section_bit(BIT_KBD_5); return false;
        case KB_TOG_KBD_6: toggle_section_bit(BIT_KBD_6); return false;

        // A 1-6
        case KB_TOG_A_1: toggle_section_bit(BIT_A_1); return false;
        case KB_TOG_A_2: toggle_section_bit(BIT_A_2); return false;
        case KB_TOG_A_3: toggle_section_bit(BIT_A_3); return false;
        case KB_TOG_A_4: toggle_section_bit(BIT_A_4); return false;
        case KB_TOG_A_5: toggle_section_bit(BIT_A_5); return false;
        case KB_TOG_A_6: toggle_section_bit(BIT_A_6); return false;

        // B 1-6
        case KB_TOG_B_1: toggle_section_bit(BIT_B_1); return false;
        case KB_TOG_B_2: toggle_section_bit(BIT_B_2); return false;
        case KB_TOG_B_3: toggle_section_bit(BIT_B_3); return false;
        case KB_TOG_B_4: toggle_section_bit(BIT_B_4); return false;
        case KB_TOG_B_5: toggle_section_bit(BIT_B_5); return false;
        case KB_TOG_B_6: toggle_section_bit(BIT_B_6); return false;

        // Display color keys (affect only display groups 0–29)
        case KB_DISP_WHT: set_display_color(255, 255, 255); return false;
        case KB_DISP_GRY: set_display_color(128, 128, 128); return false;
        case KB_DISP_RED: set_display_color(255,   0,   0); return false;
        case KB_DISP_MAG: set_display_color(255,   0, 255); return false;
        case KB_DISP_BLU: set_display_color(  0,   0, 255); return false;
        case KB_DISP_CYN: set_display_color(  0, 255, 255); return false;
        case KB_DISP_GRN: set_display_color(  0, 255,   0); return false;
        case KB_DISP_YLW: set_display_color(255, 255,   0); return false;
    }
    return true;
}

static inline bool bit_enabled(uint8_t bit) {
    return (section_mask & (1UL << bit)) != 0;
}

// Paint helper: takes already-scaled RGB (cheaper on AVR)
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

static inline uint8_t scale8(uint8_t c, uint8_t v) {
    return (uint16_t)c * v / 255;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (!section_mode) {
        return true; // normal Vial/QMK RGB Matrix behavior
    }

    // Takeover mode: clear ONLY this batch
    for (uint8_t i = led_min; i < led_max; i++) {
        rgb_matrix_set_color(i, 0, 0, 0);
    }

    // Brightness cap (prevents brownouts)
    uint8_t v = rgb_matrix_get_val();
    if (v > SECTION_MODE_MAX_VAL) v = SECTION_MODE_MAX_VAL;

    // Scaled “display color” (for LEDs 0–29 groups)
    const uint8_t d_r = scale8(disp_r, v);
    const uint8_t d_g = scale8(disp_g, v);
    const uint8_t d_b = scale8(disp_b, v);

    // Scaled number colors (unchanged behavior)
    const uint8_t kbd_r = scale8(255, v), kbd_g = scale8(255, v), kbd_b = scale8(255, v);
    const uint8_t a_r   = scale8(  0, v), a_g   = scale8( 80, v), a_b   = scale8(255, v);
    const uint8_t b_r   = scale8(255, v), b_g   = scale8(200, v), b_b   = scale8(  0, v);

    // ---- Display groups (0–29): ALL use the chosen display color ----
    if (bit_enabled(BIT_QB_LOGO))     paint_pgm_section_scaled(LEDS_QB_LOGO,    sizeof(LEDS_QB_LOGO),    led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_SCREEN))      paint_pgm_section_scaled(LEDS_SCREEN,     sizeof(LEDS_SCREEN),     led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_CH_A))        paint_pgm_section_scaled(LEDS_CH_A,       sizeof(LEDS_CH_A),       led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_CH_B))        paint_pgm_section_scaled(LEDS_CH_B,       sizeof(LEDS_CH_B),       led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_AB_MIX))      paint_pgm_section_scaled(LEDS_AB_MIX,     sizeof(LEDS_AB_MIX),     led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_AB_PREVIEW))  paint_pgm_section_scaled(LEDS_AB_PREVIEW, sizeof(LEDS_AB_PREVIEW), led_min, led_max, d_r, d_g, d_b);

    // ---- Numbers (30–47): keep your existing colors ----
    if (bit_enabled(BIT_KBD_1)) paint_pgm_section_scaled(LED_KBD_1, sizeof(LED_KBD_1), led_min, led_max, kbd_r, kbd_g, kbd_b);
    if (bit_enabled(BIT_KBD_2)) paint_pgm_section_scaled(LED_KBD_2, sizeof(LED_KBD_2), led_min, led_max, kbd_r, kbd_g, kbd_b);
    if (bit_enabled(BIT_KBD_3)) paint_pgm_section_scaled(LED_KBD_3, sizeof(LED_KBD_3), led_min, led_max, kbd_r, kbd_g, kbd_b);
    if (bit_enabled(BIT_KBD_4)) paint_pgm_section_scaled(LED_KBD_4, sizeof(LED_KBD_4), led_min, led_max, kbd_r, kbd_g, kbd_b);
    if (bit_enabled(BIT_KBD_5)) paint_pgm_section_scaled(LED_KBD_5, sizeof(LED_KBD_5), led_min, led_max, kbd_r, kbd_g, kbd_b);
    if (bit_enabled(BIT_KBD_6)) paint_pgm_section_scaled(LED_KBD_6, sizeof(LED_KBD_6), led_min, led_max, kbd_r, kbd_g, kbd_b);

    if (bit_enabled(BIT_A_1)) paint_pgm_section_scaled(LED_A_1, sizeof(LED_A_1), led_min, led_max, a_r, a_g, a_b);
    if (bit_enabled(BIT_A_2)) paint_pgm_section_scaled(LED_A_2, sizeof(LED_A_2), led_min, led_max, a_r, a_g, a_b);
    if (bit_enabled(BIT_A_3)) paint_pgm_section_scaled(LED_A_3, sizeof(LED_A_3), led_min, led_max, a_r, a_g, a_b);
    if (bit_enabled(BIT_A_4)) paint_pgm_section_scaled(LED_A_4, sizeof(LED_A_4), led_min, led_max, a_r, a_g, a_b);
    if (bit_enabled(BIT_A_5)) paint_pgm_section_scaled(LED_A_5, sizeof(LED_A_5), led_min, led_max, a_r, a_g, a_b);
    if (bit_enabled(BIT_A_6)) paint_pgm_section_scaled(LED_A_6, sizeof(LED_A_6), led_min, led_max, a_r, a_g, a_b);

    if (bit_enabled(BIT_B_1)) paint_pgm_section_scaled(LED_B_1, sizeof(LED_B_1), led_min, led_max, b_r, b_g, b_b);
    if (bit_enabled(BIT_B_2)) paint_pgm_section_scaled(LED_B_2, sizeof(LED_B_2), led_min, led_max, b_r, b_g, b_b);
    if (bit_enabled(BIT_B_3)) paint_pgm_section_scaled(LED_B_3, sizeof(LED_B_3), led_min, led_max, b_r, b_g, b_b);
    if (bit_enabled(BIT_B_4)) paint_pgm_section_scaled(LED_B_4, sizeof(LED_B_4), led_min, led_max, b_r, b_g, b_b);
    if (bit_enabled(BIT_B_5)) paint_pgm_section_scaled(LED_B_5, sizeof(LED_B_5), led_min, led_max, b_r, b_g, b_b);
    if (bit_enabled(BIT_B_6)) paint_pgm_section_scaled(LED_B_6, sizeof(LED_B_6), led_min, led_max, b_r, b_g, b_b);

    return false;
}

#endif // RGB_MATRIX_ENABLE
