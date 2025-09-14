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
#include "i2c_master.h"
#include "gpio.h"

// ---------------- GPIOs ----------------
#define INDICATOR     D4
#define SOLENOID_EN   D3
#define SOLENOID_TRIG D2

// ---------------- I2C expander base addr ----------------
#define TWI_BASE_ADDR 0x40  // 8-bit base; OR with (slave<<1)

// Example register addresses (adjust if your expander differs)
#define IN_PORT_0   0x00
#define IN_PORT_1   0x01
#define OUT_PORT_0  0x02
#define OUT_PORT_1  0x03
#define POL_INV_0   0x04
#define POL_INV_1   0x05
#define CFG_PORT_0  0x06
#define CFG_PORT_1  0x07

extern matrix_row_t matrix[MATRIX_ROWS];

// ---------------- Physical/Logical dimensions ----------------
// LOGIC_* are informational—QMK uses MATRIX_ROWS/COLS from config.h
#define LOGIC_ROWS 5
#define LOGIC_COLS 16

#define PHYS_ROWS  1
#define PHYS_COLS  66
#define PHYS_KEYS  (PHYS_ROWS * PHYS_COLS)

// ---------------- PHYS → LOGIC map ----------------
// phys2log[phys_row][phys_col] = { logic_row, logic_col }
// Valid ranges for 5x16: lr ∈ [0..4], lc ∈ [0..15]
#define UNMAPPED 0xFF, 0xFF

static const uint8_t phys2log[PHYS_ROWS][PHYS_COLS][2] = {
    {   // phys row 0
        /* [0][ 0] */ { 0,  0 },
        /* [0][ 1] */ { 0,  1 },
        /* [0][ 2] */ { 0,  2 },
        /* [0][ 3] */ { 0,  3 },
        /* [0][ 4] */ { 0,  4 },
        /* [0][ 5] */ { 0,  5 },
        /* [0][ 6] */ { 0,  6 },
        /* [0][ 7] */ { 0,  7 },
        /* [0][ 8] */ { 0,  8 },
        /* [0][ 9] */ { 0,  9 },
        /* [0][10] */ { 0, 10 },
        /* [0][11] */ { 0, 11 },
        /* [0][12] */ { 0, 12 },
        /* [0][13] */ { 0, 13 },
        /* [0][14] */ { 0, 14 },
        /* [0][15] */ { 0, 15 },

        /* [0][16] */ { 1,  0 },
        /* [0][17] */ { 1,  1 },
        /* [0][18] */ { 1,  2 },
        /* [0][19] */ { 1,  3 },
        /* [0][20] */ { 1,  4 },
        /* [0][21] */ { 1,  5 },
        /* [0][22] */ { 1,  6 },
        /* [0][23] */ { 1,  7 },
        /* [0][24] */ { 1,  8 },
        /* [0][25] */ { 1,  9 },
        /* [0][26] */ { 1, 10 },
        /* [0][27] */ { 1, 11 },
        /* [0][28] */ { 1, 12 },
        /* [0][29] */ { 1, 13 },
        /* [0][30] */ { 1, 14 },
        /* [0][31] */ { 1, 15 },

        /* [0][32] */ { 2,  0 },
        /* [0][33] */ { 2,  1 },
        /* [0][34] */ { 2,  2 },
        /* [0][35] */ { 2,  3 },
        /* [0][36] */ { 2,  4 },
        /* [0][37] */ { 2,  5 },
        /* [0][38] */ { 2,  6 },
        /* [0][39] */ { 2,  7 },
        /* [0][40] */ { 2,  8 },
        /* [0][41] */ { 2,  9 },
        /* [0][42] */ { 2, 10 },
        /* [0][43] */ { 2, 11 },
        /* [0][44] */ { 2, 12 },
        /* [0][45] */ { 2, 13 },
        /* [0][46] */ { 2, 14 },
        /* [0][47] */ { 2, 15 },

        /* [0][48] */ { 3,  0 },
        /* [0][49] */ { 3,  1 },
        /* [0][50] */ { 3,  2 },
        /* [0][51] */ { 3,  3 },
        /* [0][52] */ { 3,  4 },
        /* [0][53] */ { 3,  5 },
        /* [0][54] */ { 3,  6 },
        /* [0][55] */ { 3,  7 },
        /* [0][56] */ { 3,  8 },
        /* [0][57] */ { 3,  9 },
        /* [0][58] */ { 3, 10 },
        /* [0][59] */ { 3, 11 },
        /* [0][60] */ { 3, 12 },
        /* [0][61] */ { 3, 13 },
        /* [0][62] */ { 3, 14 },

        /* [0][63] */ { 4,  0 },
        /* [0][64] */ { 4,  1 },
        /* [0][65] */ { 4,  2 },
    }
};

// ---------------- Local state ----------------
static uint8_t prev_state[PHYS_KEYS];
static uint8_t curr_state[PHYS_KEYS];

// ---------------- Tiny I2C helper (write-then-read a register) ----------------
static inline void i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len, uint16_t timeout_ms) {
    i2c_status_t st = i2c_transmit(addr, &reg, 1, timeout_ms);  // set register pointer
    if (st != I2C_STATUS_SUCCESS) return;
    (void)i2c_receive(addr, buf, len, timeout_ms);              // read N bytes
}

// ---------------- QMK hooks ----------------
void matrix_init_user(void) {
    debug_enable = true;

    for (uint16_t i = 0; i < PHYS_KEYS; i++) {
        prev_state[i] = 0;
        curr_state[i] = 0;
    }

    // Indicator LED
    setPinOutput(INDICATOR);
    writePin(INDICATOR, 1);

    // Enable solenoid
    setPinOutput(SOLENOID_EN);
    writePin(SOLENOID_EN, 1);

    // Solenoid trigger (active high)
    setPinOutput(SOLENOID_TRIG);
    writePin(SOLENOID_TRIG, 0);

    i2c_init();
}

void matrix_clear(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix[r] = 0;
    }
}

// Read one physical key bit from an array of 5 slaves × 2 bytes each (pins[5][2])
static inline uint8_t read_phys_bit(uint16_t idx, uint8_t pins[5][2], uint8_t direct_bits) {
    // 0..7:   pins[0][0]
    // 8..15:  pins[0][1]
    // 16..23: pins[1][0]
    // 24..31: pins[1][1]
    // 32..39: pins[2][0]
    // 40..47: pins[2][1]
    // 48..55: pins[3][0]
    // 56..63: pins[3][1]
    // 64..71: pins[4][0]
    // 72..79: pins[4][1]
    // 80..81: direct bits (not needed for 66 keys, but kept for completeness)
    if (idx < 80) {
        uint8_t block = idx / 8;
        uint8_t bit   = idx % 8;
        uint8_t slave = block / 2;        // two blocks per slave
        uint8_t hi    = block % 2;        // 0 -> [0], 1 -> [1]
        return (pins[slave][hi] >> bit) & 1;
    } else {
        uint8_t bit = idx - 80;
        return (direct_bits >> bit) & 1;
    }
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    uint8_t pins[5][2] = {{0}};
    uint8_t direct = PINF & 0b11;  // if you truly wire two direct keys here

    // Read 2 input bytes from each of 5 I2C slaves
    for (uint8_t slave = 0; slave < 5; slave++) {
        i2c_read_reg((uint8_t)(TWI_BASE_ADDR | (slave << 1)), IN_PORT_0, pins[slave], 2, 10);
    }

    // Capture state & detect transitions
    uint8_t any_change = 0;
    uint8_t trigger_solenoid = 0;

    for (uint16_t p = 0; p < PHYS_KEYS; p++) {
        uint8_t last = prev_state[p];
        uint8_t now  = read_phys_bit(p, pins, direct);

        curr_state[p]   = now;
        any_change     |= (last ^ now);
        trigger_solenoid |= ((last ^ now) & now); // rising edge => key press
    }

    // Solenoid pulse + debug print when any new press occurs
    if (trigger_solenoid) {
        writePin(INDICATOR, 0);
        writePin(SOLENOID_TRIG, 1);
        wait_ms(10);
        writePin(SOLENOID_TRIG, 0);
        wait_ms(40);
        writePin(INDICATOR, 1);

        // Optional debug
        for (uint8_t slave = 0; slave < 5; slave++) {
            if (pins[slave][0] | pins[slave][1]) {
                uprintf("SLAVE %u - %08b:%08b\n", slave, pins[slave][0], pins[slave][1]);
            }
        }
        uprintf("DIRECT - %02b\n", direct);
    }

    // Build the logical matrix bitfields from phys→logic map
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        current_matrix[r] = 0;
    }

    for (uint16_t p = 0; p < PHYS_KEYS; p++) {
        uint8_t lr = phys2log[0][p][0];
        uint8_t lc = phys2log[0][p][1];
        if (lr == 0xFF || lc == 0xFF) continue;                 // unmapped
        if (lr >= MATRIX_ROWS || lc >= MATRIX_COLS) continue;   // safety
        if (curr_state[p]) {
            current_matrix[lr] |= (matrix_row_t)(1u << lc);
        }
    }

    // Move curr→prev for next scan
    for (uint16_t p = 0; p < PHYS_KEYS; p++) {
        prev_state[p] = curr_state[p];
    }

    return any_change;
}
