#pragma once

#define VIAL_KEYBOARD_UID {0x0E, 0xC1, 0x73, 0x23, 0x9F, 0x8F, 0x8F, 0x2C}

#define RGB_MATRIX_DISABLE_SHARED_KEYCODES // Indapendent Control of both LED Areas

#define RGB_MATRIX_SLEEP // turn off effects when suspended
#define RGBLIGHT_SLEEP // turn off effects when suspended

//Defaults for the Badge Heart
#define RGBLIGHT_DEFAULT_MODE RGBLIGHT_MODE_BREATHING
#define RGBLIGHT_DEFAULT_HUE 170
#define RGBLIGHT_DEFAULT_SAT UINT8_MAX
#define RGBLIGHT_DEFAULT_VAL RGBLIGHT_LIMIT_VAL
#define RGBLIGHT_DEFAULT_SPD 3
#define RGBLIGHT_DEFAULT_ON true


//Defaults for Back Left Accent
#define VIALRGB_NO_DIRECT
#define RGB_MATRIX_TYPING_HEATMAP_SPREAD 255

//#define RGB_MATRIX_DEFAULT_MODE
