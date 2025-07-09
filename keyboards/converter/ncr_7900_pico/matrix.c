#include "quantum.h"
#include "print.h"      // for xprintf()
#include "uart.h"       // QMK UART API
#include "config.h"     // SERIAL_UART_BAUD, IDLE_CODE, MATRIX_ROWS

// — scan→pos map (fill with your final values; 0xFF = ignore) —
static const uint8_t sc_to_pos[256] = {
    [0x63] = 0x00,
    [0x33] = 0x10,
    [0x66] = 0x01,
    [0x19] = 0x11,
    // ... continue mapping your codes ...
};

static matrix_row_t matrix[MATRIX_ROWS];
static uint8_t last_sc = 0xFF;

// Initialize QMK UART with optional inversion configured via config.h/mcuconf.h
static void init_uart(void) {
    uart_init(SERIAL_UART_BAUD);
}

void matrix_init(void) {
    init_uart();
    // clear matrix state
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix[r] = 0;
    }
    last_sc = 0xFF;
}

uint8_t matrix_scan(void) {
    // Heartbeat detection state
    static bool hb_first = false;
    static uint32_t hb_count = 0;

    // Read all available serial bytes
    while (uart_available()) {
        // Read a byte (already inverted if RX inversion is enabled in mcuconf)
        uint8_t raw = uart_read();

        // 1) Heartbeat: detect FF → IDLE_CODE sequence
        if (!hb_first) {
            if (raw == 0xFF) {
                hb_first = true;
                continue;
            }
        } else {
            hb_first = false;
            if (raw == IDLE_CODE) {
                hb_count++;
                // Optional: xprintf("Heartbeat #%u\n", hb_count);
                continue;
            }
        }

        // 2) Log raw and scan code for hid_listen
        xprintf("RAW:%02X SC:%02X ", raw, raw);

        // 3) Map scan code to matrix position
        uint8_t pos = sc_to_pos[raw];
        if (pos == 0xFF) {
            xprintf("→IGNORE\n");
            continue;
        }

        uint8_t row = pos >> 4;
        uint8_t col = pos & 0x0F;

        // 4) Release previous key if different
        if (last_sc != 0xFF && last_sc != raw) {
            uint8_t old = sc_to_pos[last_sc];
            matrix[old >> 4] &= ~(1u << (old & 0x0F));
        }

        // 5) Press new key
        matrix[row] |= (1u << col);
        last_sc = raw;

        // 6) Log make event
        xprintf("→MAKE row%u,col%u\n", row, col);
    }

    return 0;
}

matrix_row_t matrix_get_row(uint8_t row) {
    return matrix[row];
}

void matrix_print(void) { }
