#include QMK_KEYBOARD_H
#include "print.h"  // for xprintf()

// Forward-declared from matrix.c
void serial_write(uint8_t b);
extern void walt_send_led_mask(uint8_t mask);

// Layers
enum layers {
    _BASE,
    _FN
};

// Sound modes
typedef enum {
    SOUND_NONE  = 0x00,
    SOUND_CLICK = 0x20,
    SOUND_BEEP  = 0x40,
} sound_mode_t;

static sound_mode_t current_sound_mode = SOUND_NONE;

enum custom_keycodes {
    CLICKER = SAFE_RANGE,
    SUPER_BEEPER,
    SILENT,
};





const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

	[_BASE] = LAYOUT(
    _______, _______,     _______, _______, SILENT,                   CLICKER, SUPER_BEEPER,                            _______, _______, _______,                      _______, _______, _______,
    _______, _______,                                                                                                                                               _______, _______, _______,
    _______, _______,     _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,    _______, _______, _______,
    _______, _______,     _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,    _______, _______, _______,
    _______, _______,     _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,             _______, _______, _______,
    _______, _______,     _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,             _______, _______, _______,
    _______, _______,                       _______,                            _______,                   _______,                   _______,                      _______, _______, _______
        ),
    [_FN] = LAYOUT(
    _______, _______,     _______, _______, _______,                   _______, _______,                            _______, _______, _______,                      _______, _______, _______,
    _______, _______,                                                                                                                                               _______, _______, _______,
    _______, _______,     _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,    _______, _______, _______,
    _______, _______,     _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,    _______, _______, _______,
    _______, _______,     _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,             _______, _______, _______,
    _______, _______,     _______,          _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,          _______,             _______, _______, _______,
    _______, _______,                       _______,                            _______,                   _______,                   _______,                      _______, _______, _______
        ),

};
// LED update called by QMK automatically
bool led_update_kb(led_t led_state) {
    uint8_t mask = 0;
    if (led_state.caps_lock) mask |= 0x01;
    if (layer_state_is(_FN)) mask |= 0x02;

    xprintf("WALT LED mask = 0x%02X\n", mask);
    walt_send_led_mask(mask);  // only low nibble
    return false;
}

// Trigger LED update when layer changes
layer_state_t layer_state_set_user(layer_state_t state) {
    led_update_kb(host_keyboard_led_state());
    return state;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case CLICKER:
                if (current_sound_mode == SOUND_CLICK) {
                    current_sound_mode = SOUND_NONE;
                } else {
                    current_sound_mode = SOUND_CLICK;
                }
                return false;

            case SUPER_BEEPER:
                if (current_sound_mode == SOUND_BEEP) {
                    current_sound_mode = SOUND_NONE;
                } else {
                    current_sound_mode = SOUND_BEEP;
                }
                return false;

            case SILENT:
                current_sound_mode = SOUND_NONE;
                return false;

            case KC_F:
                layer_invert(_FN);
                break;
        }

        // Compute LED mask (including KC_F flip simulation)
        uint8_t mask = 0;
        if (host_keyboard_led_state().caps_lock) mask |= 0x01;

        bool fn_active = layer_state_is(_FN);
        if (keycode == KC_F) fn_active = !fn_active;
        if (fn_active) mask |= 0x02;

        // Send TX code based on mode + mask
        uint8_t tx_byte = current_sound_mode | mask;
        xprintf("TX: 0x%02X\n", tx_byte);
        serial_write(tx_byte);

        if (keycode == KC_F) return false;
    }

    return true;
}
