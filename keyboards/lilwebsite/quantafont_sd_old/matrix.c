#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "wait.h"
#include "action_layer.h"
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "led.h"
#include <util/atomic.h>

#define INDICATOR B7

extern matrix_row_t matrix[MATRIX_ROWS];

void matrix_init_user(void)
{
	debug_enable = true;
	debug_matrix = true;

	// indicator LED
	setPinOutput(INDICATOR);
	writePin(INDICATOR, 1);

	setPinOutput(F0); // A
	setPinOutput(F1); // B
	setPinOutput(F4); // C
	setPinOutput(F5); // D

	// inputs
	DDRD = 0x00;
	PORTD = 0xFF;
}

void matrix_clear(void)
{
	for (uint8_t x = 0; x < MATRIX_ROWS; x++)
	{matrix[x] = 0;}
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
	uint8_t state = 0;

	for (uint8_t r = 0; r < MATRIX_ROWS; r++)
	{
		matrix_row_t last_state = current_matrix[r];

		writePin(F0, r & 1);
		writePin(F1, (r >> 1) & 1);
		writePin(F4, (r >> 2) & 1);
		writePin(F5, (r >> 3) & 1);

		wait_us(20);

		matrix_row_t pins = PIND;

		if (last_state != pins)
		{state = 1;}
		//{uprintf("ROW %d - %08b\n", r, pins);}

		current_matrix[r] = pins;
	}

	if (state)
	{
		writePin(INDICATOR, 0);
		wait_ms(50);
		writePin(INDICATOR, 1);
	}

	return state;
}
