#ifdef RGB_MATRIX_ENABLE
#include "drivers/led/issi/is31fl3236.h"

const is31fl3236_led_t PROGMEM g_is31fl3236_leds[IS31FL3236_LED_COUNT] = {
    { 0, OUT34, OUT35, OUT36 },
    { 0, OUT31, OUT32, OUT33 },
    { 0, OUT28, OUT29, OUT30 },
    { 0, OUT25, OUT26, OUT27 },
    { 0, OUT22, OUT23, OUT24 },
    { 0, OUT19, OUT20, OUT21 },
    { 0, OUT16, OUT17, OUT18 },
    { 0, OUT13, OUT14, OUT15 },
    { 0, OUT10, OUT11, OUT12 },
    { 0, OUT7,  OUT8,  OUT9  },
    { 0, OUT4,  OUT5,  OUT6  },
    { 0, OUT1,  OUT2,  OUT3  },
};

#endif  // RGB_MATRIX_ENABLE
