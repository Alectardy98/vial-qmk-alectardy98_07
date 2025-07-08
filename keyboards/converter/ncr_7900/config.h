#pragma once

/* key matrix size */
#define MATRIX_ROWS 11
#define MATRIX_COLS 11

/* — NCR-7900 serial settings — */
#define SERIAL_UART_BAUD 1200  /* must match the keyboard’s 1200 baud */

/* — protocol constants — */
#define IDLE_CODE   0x5F    // filler “00→5F” idle frame
#define BREAK_DELTA 0x10    // many keys send (make−0x10) as break

/* NCR-7900 heartbeat & break logic */
#define IDLE_CODE    0x5F    // filler idle byte
#define BREAK_DELTA  0x10    // make→break = code−0x10
