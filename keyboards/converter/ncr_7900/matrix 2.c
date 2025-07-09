#include <avr/io.h>
#include "quantum.h"
#include "print.h"   // for xprintf()
#include "config.h"    // SERIAL_UART_BAUD, IDLE_CODE, BREAK_DELTA, MATRIX_ROWS

// — scan→pos map (fill with your final values; 0xFF = ignore) —
static const uint8_t sc_to_pos[256] = {
    [0x63] = 0x00,
    [0x33] = 0x10,
    [0x66] = 0x01,
    [0x19] = 0x11,
    [0x31] = 0x02,
    [0x62] = 0x12,
    [0x18] = 0x03,
    [0x61] = 0x13,
    [0x30] = 0x04,
    [0x60] = 0x14,
    [0x01] = 0x05,
    [0x5F] = 0x15,  // real 0x5F key if standalone
    [0x2F] = 0x06,
    [0x5E] = 0x16,
    [0x17] = 0x07,
    [0x5D] = 0x17,
    [0x2E] = 0x08,
    [0x5C] = 0x18,
    [0x0B] = 0x09,
    [0x5B] = 0x19,
    [0x2D] = 0x0A,
    [0x5A] = 0x1A,
    [0x4E] = 0x20,
    [0x79] = 0x30,
    [0x7F] = 0x21,
    [0x3F] = 0x31,
    [0x7E] = 0x22,
    [0x1F] = 0x32,
    [0x7D] = 0x23,
    [0x3E] = 0x33,
    [0x7C] = 0x24,
    [0x0F] = 0x34,
    [0x7B] = 0x25,
    [0x3D] = 0x35,
    [0x7A] = 0x26,
    [0x1E] = 0x36,
    [0x26] = 0x27,
    [0x52] = 0x37,
    [0x14] = 0x28,
    [0x54] = 0x38,
    [0x0A] = 0x29,
    [0x53] = 0x39,
    [0x2C] = 0x2A,
    [0x13] = 0x40,
    [0x4C] = 0x50,
    [0x3C] = 0x41,
    [0x78] = 0x51,
    [0x71] = 0x42,
    [0x38] = 0x52,
    [0x70] = 0x43,
    [0x07] = 0x53,
    [0x03] = 0x44,
    [0x6F] = 0x54,
    [0x37] = 0x45,
    [0x6E] = 0x55,
    [0x77] = 0x46,
    [0x3B] = 0x56,
    [0x09] = 0x47,
    [0x51] = 0x57,
    [0x28] = 0x48,
    [0x15] = 0x58,
    [0x55] = 0x49,
    [0x2A] = 0x59,
    [0x58] = 0x4A,
    [0x65] = 0x60,
    [0x32] = 0x70,
    [0x76] = 0x61,
    [0x1B] = 0x71,
    [0x6D] = 0x62,
    [0x36] = 0x72,
    [0x6C] = 0x63,
    [0x0D] = 0x73,
    [0x6B] = 0x64,
    [0x35] = 0x74,
    [0x6A] = 0x65,
    [0x1D] = 0x75,
    [0x75] = 0x66,
    [0x3A] = 0x76,
    [0x50] = 0x77,
    [0x02] = 0x68,
    [0x57] = 0x78,
    [0x2B] = 0x69,
    [0x56] = 0x79,
    [0x29] = 0x6A,
    [0x4D] = 0x80,
    [0x64] = 0x90,
    [0x74] = 0x81,
    [0x0E] = 0x91,
    [0x1A] = 0x82,
    [0x69] = 0x92,
    [0x34] = 0x83,
    [0x68] = 0x93,
    [0x06] = 0x84,
    [0x73] = 0x94,
    [0x39] = 0x85,
    [0x72] = 0x95,
    [0x1C] = 0x86,
    [0x0C] = 0x96,
    [0x4B] = 0x87,
    [0x4F] = 0x97,
    [0x27] = 0x88,
    [0x05] = 0x98,
    [0x16] = 0x89,
    [0x59] = 0x99,
    [0x25] = 0xA1,
    [0x67] = 0xA3,
    [0x4A] = 0xA6,
    [0x12] = 0xA7,
};

static matrix_row_t matrix[MATRIX_ROWS];
static uint8_t      last_sc       = 0xFF;
static bool         saw_zero      = false;
static bool         pending_corrupt = false;

// UART1 init @ SERIAL_UART_BAUD
static inline void uart_init(void) {
    uint16_t ubrr = (F_CPU / (16UL * SERIAL_UART_BAUD)) - 1;
    UBRR1L = (uint8_t)ubrr;
    UBRR1H = (uint8_t)(ubrr >> 8);
    UCSR1A = 0;
    UCSR1B = _BV(RXEN1);
    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);
}
static inline bool    uart_avail(void) { return UCSR1A & _BV(RXC1); }
static inline uint8_t uart_read(void)   { return UDR1; }

void matrix_init(void) {
    uart_init();
    xprintf(">> MATRIX INIT\n");
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix[r] = 0;
    }
}

uint8_t matrix_scan(void) {
    while (uart_avail()) {
        uint8_t raw = uart_read();
        bool is_corrupt = raw & 0x80;
        uint8_t sc = raw & 0x7F;

        // drop corrupted-idle 5F
        if (pending_corrupt && raw == IDLE_CODE) {
            pending_corrupt = false;
            continue;
        }
        pending_corrupt = is_corrupt;

        // handle any corrupted byte as a make of its masked code
        if (is_corrupt) {
            uint8_t pos = sc_to_pos[sc];
            if (pos != 0xFF) {
                uint8_t row = pos >> 4;
                uint8_t col = pos & 0x0F;
                xprintf("CORRUPT RAW:%02X →MAKE row%d,col%d\n", raw, row, col);
                matrix[row] |= (1u << col);
                last_sc = sc;
            }
            continue;
        }

        // 1) skip filler zero
        if (raw == 0x00) {
            saw_zero = true;
            continue;
        }
        // 2) heartbeat idle: release last press-only key
        if (raw == IDLE_CODE && saw_zero) {
            if (last_sc != 0xFF) {
                uint8_t pos = sc_to_pos[last_sc];
                matrix[pos >> 4] &= ~(1u << (pos & 0x0F));
                last_sc = 0xFF;
            }
            saw_zero = false;
            continue;
        }
        saw_zero = false;

        // mask noise and now treat sc = raw
        sc = raw;
        xprintf("RAW:%02X SC:%02X ", raw, sc);

        // 3) explicit break?
        if (last_sc != 0xFF && sc == (uint8_t)(last_sc - BREAK_DELTA)) {
            xprintf("→BREAK\n");
            uint8_t pos = sc_to_pos[last_sc];
            matrix[pos >> 4] &= ~(1u << (pos & 0x0F));
            last_sc = 0xFF;
            continue;
        }

        // 4) make event
        {
            uint8_t pos = sc_to_pos[sc];
            if (pos == 0xFF) {
                xprintf("→IGNORE\n");
                continue;
            }
            uint8_t row = pos >> 4;
            uint8_t col = pos & 0x0F;
            xprintf("→MAKE row%d,col%d\n", row, col);
            // release previous
            if (last_sc != 0xFF && last_sc != sc) {
                uint8_t old = sc_to_pos[last_sc];
                matrix[old >> 4] &= ~(1u << (old & 0x0F));
            }
            // press new
            matrix[row] |= (1u << col);
            last_sc = sc;
        }
    }
    return 0;
}

matrix_row_t matrix_get_row(uint8_t row) {
    return matrix[row];
}

void matrix_print(void) { }
