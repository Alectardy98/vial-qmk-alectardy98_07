#include "ec_calibration_defaults.h"
#include "ec_switch_matrix.h"

// Provided by your firmware
extern eeprom_ec_config_t eeprom_ec_config;
extern ec_config_t        ec_config;

void ec_apply_calibration_defaults(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        for (uint8_t c = 0; c < MATRIX_COLS; c++) {
            // noise_floor lives in ec_config
            ec_config.noise_floor[r][c] = EC_NOISE_FLOOR[r][c];

            // bottoming_reading lives in eeprom struct
            eeprom_ec_config.bottoming_reading[r][c] = EC_BOTTOMING[r][c];

            // rescaled APC thresholds live in ec_config
            ec_config.rescaled_mode_0_actuation_threshold[r][c] = EC_APC_ACT[r][c];
            ec_config.rescaled_mode_0_release_threshold[r][c]   = EC_APC_REL[r][c];

            // Rapid Trigger initial deadzone field name in your project:
            ec_config.rescaled_mode_1_initial_deadzone_offset[r][c] = EC_RT_DZ[r][c];
        }
    }

    // Keep the base thresholds consistent with your logs
    ec_config.mode_0_actuation_threshold = 550;
    ec_config.mode_0_release_threshold   = 500;
}
