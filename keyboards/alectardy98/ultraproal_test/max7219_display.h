#pragma once
#include <stdint.h>

void max7219_init(void);
void max7219_set_digit(uint8_t pos, uint8_t value);
