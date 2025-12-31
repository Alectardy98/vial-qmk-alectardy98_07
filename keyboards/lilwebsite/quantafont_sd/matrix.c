#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

#include "matrix.h"
#include "wait.h"
#include "gpio.h"

// Keep the global matrix from QMK
extern matrix_row_t matrix[MATRIX_ROWS];

// Row-address outputs (your decoder/select lines)
#define ROW_A F0
#define ROW_B F1
#define ROW_C F4
#define ROW_D F5

static inline void select_row(uint8_t r) {
    writePin(ROW_A, (r >> 0) & 1);
    writePin(ROW_B, (r >> 1) & 1);
    writePin(ROW_C, (r >> 2) & 1);
    writePin(ROW_D, (r >> 3) & 1);
}

// QMK calls this (not matrix_init_user) for custom matrices
void matrix_init_custom(void) {
    // row address outputs
    setPinOutput(ROW_A);
    setPinOutput(ROW_B);
    setPinOutput(ROW_C);
    setPinOutput(ROW_D);

    // default row select = 0
    select_row(0);

    // column inputs with pull-ups on PORTD (D0..D7)
    DDRD  = 0x00;
    PORTD = 0xFF;
}

void matrix_clear(void) {
    for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
        matrix[i] = 0;
    }
}

// Returns true if the matrix changed
bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;

    matrix_row_t new_matrix[MATRIX_ROWS] = {0};

    // Scan all physical rows (0..MATRIX_COLS-1) via address lines
    for (uint8_t r = 0; r < MATRIX_COLS; r++) {
        select_row(r);
        wait_us(30);  // settle time

        // With pull-ups: idle=1, pressed=0 => invert so pressed becomes 1
        uint8_t pins = (uint8_t)~PIND;

        // TRANSPOSE (keep your “rows/cols reversed” behavior):
        // each PORTD bit (column) becomes a "row index" in QMK,
        // and the selected row number becomes the bit position.
        for (uint8_t c = 0; c < MATRIX_ROWS; c++) {
            if (pins & (1u << c)) {
                new_matrix[c] |= ((matrix_row_t)1u << r);
            }
        }
    }

    // compare and update
    for (uint8_t c = 0; c < MATRIX_ROWS; c++) {
        if (current_matrix[c] != new_matrix[c]) {
            current_matrix[c] = new_matrix[c];
            changed = true;
        }
    }

    return changed;
}
