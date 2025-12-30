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

    // row address outputs
    setPinOutput(F0); // A
    setPinOutput(F1); // B
    setPinOutput(F4); // C
    setPinOutput(F5); // D

    // column inputs with pull-ups
    DDRD  = 0x00;
    PORTD = 0xFF;
}

void matrix_clear(void)
{
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        matrix[x] = 0;
    }
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool state_changed = false;

    // temporary storage per column
    matrix_row_t new_matrix[MATRIX_ROWS];
    for (uint8_t c = 0; c < MATRIX_ROWS; c++) {
        new_matrix[c] = 0;
    }

    // scan all physical rows (driven by F0/F1/F4/F5)
    for (uint8_t r = 0; r < MATRIX_COLS; r++) {
        // drive row address
        writePin(F0, (r >> 0) & 1);
        writePin(F1, (r >> 1) & 1);
        writePin(F4, (r >> 2) & 1);
        writePin(F5, (r >> 3) & 1);

        wait_us(20);

        // read all physical columns
        uint8_t pins = PIND;

        // transpose: each column bit goes into matrix[col] with row-bit set
        for (uint8_t c = 0; c < MATRIX_ROWS; c++) {
            if (pins & (1 << c)) {
                new_matrix[c] |= (1 << r);
            }
        }
    }

    // compare against old and update
    for (uint8_t c = 0; c < MATRIX_ROWS; c++) {
        if (current_matrix[c] != new_matrix[c]) {
            state_changed = true;
            current_matrix[c] = new_matrix[c];
        }
    }


    return state_changed;
}
