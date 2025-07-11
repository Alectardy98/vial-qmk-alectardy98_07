#pragma once
#include <stdbool.h>
#include <stdint.h>

void bar_led_init(void);
void bar_led_write(uint8_t val);
void bar_led_set(uint8_t bar, bool on);
void bar_led_toggle(uint8_t bar);
