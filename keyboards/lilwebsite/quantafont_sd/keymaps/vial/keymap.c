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

#ifdef __AVR__
#    include <avr/pgmspace.h>
#endif

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
// A: 36=5,37=3,38=1,39=2,40=4,41=6
static const uint8_t PROGMEM SEQ_A[6]   = { 36, 37, 38, 39, 40, 41 };
// B: 42=5,43=3,44=1,45=2,46=4,47=6
static const uint8_t PROGMEM SEQ_B[6]   = { 42, 43, 44, 45, 46, 47 };

// --- Section bit positions (ONLY the 6 display toggles now) ---
enum section_bits {
    BIT_QB_LOGO = 0,
    BIT_SCREEN,
    BIT_CH_A,
    BIT_CH_B,
    BIT_AB_MIX,
    BIT_AB_PREVIEW,
};

static bool    section_mode = false;
static uint8_t section_mask = 0; // 6 bits is enough

// One “display color” used for ALL display LEDs (0–29)
static uint8_t disp_r = 255;
static uint8_t disp_g = 0;
static uint8_t disp_b = 0;

static inline void set_display_color(uint8_t r, uint8_t g, uint8_t b) {
    disp_r = r; disp_g = g; disp_b = b;
}

static inline void toggle_section_bit(uint8_t bit) {
    section_mask ^= (1u << bit);
}

static inline bool bit_enabled(uint8_t bit) {
    return (section_mask & (1u << bit)) != 0;
}

// ---------- Persist section_mask + disp RGB in eeconfig_user() ----------
// Layout (32-bit):
// bits 0..5  = section_mask
// bits 6..13 = disp_r
// bits 14..21= disp_g
// bits 22..29= disp_b
static inline uint32_t pack_user_cfg(void) {
    uint32_t v = 0;
    v |= (uint32_t)(section_mask & 0x3F);
    v |= ((uint32_t)disp_r) << 6;
    v |= ((uint32_t)disp_g) << 14;
    v |= ((uint32_t)disp_b) << 22;
    return v;
}

static inline void unpack_user_cfg(uint32_t v) {
    section_mask = (uint8_t)(v & 0x3F);
    disp_r = (uint8_t)((v >> 6)  & 0xFF);
    disp_g = (uint8_t)((v >> 14) & 0xFF);
    disp_b = (uint8_t)((v >> 22) & 0xFF);

    // if user cfg was never set, it might come in as 0's — pick a sane default
    if (disp_r == 0 && disp_g == 0 && disp_b == 0) {
        set_display_color(255, 0, 0);
    }
}

static inline void save_user_cfg(void) {
    eeconfig_update_user(pack_user_cfg());
}

void keyboard_post_init_user(void) {
    uint32_t u = eeconfig_read_user();
    if (u == 0xFFFFFFFFu) {
        // uninitialized user dword: set defaults WITHOUT wiping Vial’s EEPROM
        section_mask = 0;
        set_display_color(255, 0, 0);
        save_user_cfg();
    } else {
        unpack_user_cfg(u);
    }
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
                rgb_matrix_enable_noeeprom();
                rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);

                // cap brightness in section mode (brownout protection)
                uint8_t v = rgb_matrix_get_val();
                if (v > SECTION_MODE_MAX_VAL) v = SECTION_MODE_MAX_VAL;
                rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), v);
            }
            return false;

        case KB_CLR_SECS:
            section_mask = 0;
            save_user_cfg();
            return false;

        case KB_TOG_LOGO:    toggle_section_bit(BIT_QB_LOGO);    save_user_cfg(); return false;
        case KB_TOG_SCREEN:  toggle_section_bit(BIT_SCREEN);     save_user_cfg(); return false;
        case KB_TOG_CH_A:    toggle_section_bit(BIT_CH_A);       save_user_cfg(); return false;
        case KB_TOG_CH_B:    toggle_section_bit(BIT_CH_B);       save_user_cfg(); return false;
        case KB_TOG_AB_MIX:  toggle_section_bit(BIT_AB_MIX);     save_user_cfg(); return false;
        case KB_TOG_AB_PREV: toggle_section_bit(BIT_AB_PREVIEW); save_user_cfg(); return false;

        case KB_DISP_WHT: set_display_color(255, 255, 255); save_user_cfg(); return false;
        case KB_DISP_GRY: set_display_color(128, 128, 128); save_user_cfg(); return false;
        case KB_DISP_RED: set_display_color(255,   0,   0); save_user_cfg(); return false;
        case KB_DISP_MAG: set_display_color(255,   0, 255); save_user_cfg(); return false;
        case KB_DISP_BLU: set_display_color(  0,   0, 255); save_user_cfg(); return false;
        case KB_DISP_CYN: set_display_color(  0, 255, 255); save_user_cfg(); return false;
        case KB_DISP_GRN: set_display_color(  0, 255,   0); save_user_cfg(); return false;
        case KB_DISP_YLW: set_display_color(255, 255,   0); save_user_cfg(); return false;
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

// ---- Number animation state ----
#ifndef NUM_STEP_MS
#    define NUM_STEP_MS 250
#endif

static uint32_t num_timer = 0;
static uint8_t  num_step  = 0;

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (!section_mode) return true;

    // Clear ONLY this batch
    for (uint8_t i = led_min; i < led_max; i++) {
        rgb_matrix_set_color(i, 0, 0, 0);
    }

    // Update animation step
    if (timer_elapsed32(num_timer) >= NUM_STEP_MS) {
        num_timer = timer_read32();
        num_step++;
        if (num_step >= 6) num_step = 0;
    }

    // Brightness cap
    uint8_t v = rgb_matrix_get_val();
    if (v > SECTION_MODE_MAX_VAL) v = SECTION_MODE_MAX_VAL;

    // Display color scaled
    const uint8_t d_r = scale8(disp_r, v);
    const uint8_t d_g = scale8(disp_g, v);
    const uint8_t d_b = scale8(disp_b, v);

    // Number colors (pick what you want; these keep your old intent)
    const uint8_t kbd_r = scale8(255, v), kbd_g = scale8(255, v), kbd_b = scale8(255, v);
    const uint8_t a_r   = scale8(  0, v), a_g   = scale8( 80, v), a_b   = scale8(255, v);
    const uint8_t b_r   = scale8(255, v), b_g   = scale8(200, v), b_b   = scale8(  0, v);

    // ---- Display groups (0–29): ALL use chosen display color ----
    if (bit_enabled(BIT_QB_LOGO))    paint_pgm_section_scaled(LEDS_QB_LOGO,    sizeof(LEDS_QB_LOGO),    led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_SCREEN))     paint_pgm_section_scaled(LEDS_SCREEN,     sizeof(LEDS_SCREEN),     led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_CH_A))       paint_pgm_section_scaled(LEDS_CH_A,       sizeof(LEDS_CH_A),       led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_CH_B))       paint_pgm_section_scaled(LEDS_CH_B,       sizeof(LEDS_CH_B),       led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_AB_MIX))     paint_pgm_section_scaled(LEDS_AB_MIX,     sizeof(LEDS_AB_MIX),     led_min, led_max, d_r, d_g, d_b);
    if (bit_enabled(BIT_AB_PREVIEW)) paint_pgm_section_scaled(LEDS_AB_PREVIEW, sizeof(LEDS_AB_PREVIEW), led_min, led_max, d_r, d_g, d_b);

    // ---- Numbers: automatic “one-at-a-time” sequences ----
    if (bit_enabled(BIT_SCREEN)) paint_single_from_seq(SEQ_KBD, num_step, led_min, led_max, kbd_r, kbd_g, kbd_b);
    if (bit_enabled(BIT_CH_A))   paint_single_from_seq(SEQ_A,   num_step, led_min, led_max, a_r,   a_g,   a_b);
    if (bit_enabled(BIT_CH_B))   paint_single_from_seq(SEQ_B,   num_step, led_min, led_max, b_r,   b_g,   b_b);

    return false;
}

#endif // RGB_MATRIX_ENABLE
