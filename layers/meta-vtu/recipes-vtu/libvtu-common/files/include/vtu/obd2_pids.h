/**
 * @file obd2_pids.h
 * @brief OBD-II Parameter IDs (PIDs) definitions
 * 
 * Based on SAE J1979 / ISO 15031-5 standards.
 * Mode 01 = Current Data, Mode 03 = DTCs, Mode 09 = Vehicle Info
 */

#ifndef VTU_OBD2_PIDS_H
#define VTU_OBD2_PIDS_H

#include <stdint.h>

/*============================================================================
 * OBD-II Service Modes (SIDs)
 *===========================================================================*/

#define OBD2_MODE_CURRENT_DATA      0x01    /* Show current data */
#define OBD2_MODE_FREEZE_FRAME      0x02    /* Show freeze frame data */
#define OBD2_MODE_READ_DTC          0x03    /* Show stored DTCs */
#define OBD2_MODE_CLEAR_DTC         0x04    /* Clear DTCs and freeze frame */
#define OBD2_MODE_TEST_RESULTS_O2   0x05    /* Oxygen sensor test results */
#define OBD2_MODE_TEST_RESULTS      0x06    /* Test results (non-continuous) */
#define OBD2_MODE_PENDING_DTC       0x07    /* Show pending DTCs */
#define OBD2_MODE_CONTROL           0x08    /* Control on-board system */
#define OBD2_MODE_VEHICLE_INFO      0x09    /* Request vehicle information */
#define OBD2_MODE_PERMANENT_DTC     0x0A    /* Permanent DTCs */

/* Response offset (response SID = request SID + 0x40) */
#define OBD2_RESPONSE_OFFSET        0x40

/*============================================================================
 * Mode 01 - Current Data PIDs
 *===========================================================================*/

#define OBD2_PID_SUPPORTED_01_20    0x00    /* Supported PIDs [01-20] bitmap */
#define OBD2_PID_MONITOR_STATUS     0x01    /* Monitor status since DTC cleared */
#define OBD2_PID_FREEZE_DTC         0x02    /* DTC that caused freeze frame */
#define OBD2_PID_FUEL_SYSTEM        0x03    /* Fuel system status */
#define OBD2_PID_ENGINE_LOAD        0x04    /* Calculated engine load */
#define OBD2_PID_COOLANT_TEMP       0x05    /* Engine coolant temperature */
#define OBD2_PID_SHORT_FUEL_TRIM_1  0x06    /* Short term fuel trim - Bank 1 */
#define OBD2_PID_LONG_FUEL_TRIM_1   0x07    /* Long term fuel trim - Bank 1 */
#define OBD2_PID_SHORT_FUEL_TRIM_2  0x08    /* Short term fuel trim - Bank 2 */
#define OBD2_PID_LONG_FUEL_TRIM_2   0x09    /* Long term fuel trim - Bank 2 */
#define OBD2_PID_FUEL_PRESSURE      0x0A    /* Fuel pressure (gauge) */
#define OBD2_PID_INTAKE_MAP         0x0B    /* Intake manifold pressure */
#define OBD2_PID_ENGINE_RPM         0x0C    /* Engine RPM */
#define OBD2_PID_VEHICLE_SPEED      0x0D    /* Vehicle speed */
#define OBD2_PID_TIMING_ADVANCE     0x0E    /* Timing advance */
#define OBD2_PID_INTAKE_TEMP        0x0F    /* Intake air temperature */
#define OBD2_PID_MAF                0x10    /* MAF air flow rate */
#define OBD2_PID_THROTTLE_POS       0x11    /* Throttle position */
#define OBD2_PID_O2_SENSORS         0x13    /* Oxygen sensors present */
#define OBD2_PID_OBD_STANDARD       0x1C    /* OBD standard compliance */
#define OBD2_PID_RUN_TIME           0x1F    /* Run time since engine start */
#define OBD2_PID_SUPPORTED_21_40    0x20    /* Supported PIDs [21-40] bitmap */
#define OBD2_PID_FUEL_LEVEL         0x2F    /* Fuel tank level input */
#define OBD2_PID_SUPPORTED_41_60    0x40    /* Supported PIDs [41-60] bitmap */
#define OBD2_PID_AMBIENT_TEMP       0x46    /* Ambient air temperature */
#define OBD2_PID_OIL_TEMP           0x5C    /* Engine oil temperature */

/*============================================================================
 * Mode 09 - Vehicle Information PIDs
 *===========================================================================*/

#define OBD2_PID_VIN_COUNT          0x01    /* VIN message count */
#define OBD2_PID_VIN                0x02    /* Vehicle Identification Number */
#define OBD2_PID_CALID_COUNT        0x03    /* Calibration ID message count */
#define OBD2_PID_CALID              0x04    /* Calibration ID */
#define OBD2_PID_ECU_NAME           0x0A    /* ECU name */

/*============================================================================
 * PID Conversion Formulas
 * 
 * Each function converts raw OBD-II response bytes to physical values.
 * A, B, C, D refer to data bytes in the response.
 *===========================================================================*/

/**
 * @brief Convert engine load (PID 0x04)
 * @param a Data byte A
 * @return Load percentage (0-100%)
 */
static inline float obd2_calc_engine_load(uint8_t a) {
    return (float)a * 100.0f / 255.0f;
}

/**
 * @brief Convert coolant temperature (PID 0x05)
 * @param a Data byte A
 * @return Temperature in °C (-40 to 215)
 */
static inline int obd2_calc_coolant_temp(uint8_t a) {
    return (int)a - 40;
}

/**
 * @brief Convert engine RPM (PID 0x0C)
 * @param a Data byte A (high byte)
 * @param b Data byte B (low byte)
 * @return RPM (0-16383.75)
 */
static inline float obd2_calc_rpm(uint8_t a, uint8_t b) {
    return ((float)a * 256.0f + (float)b) / 4.0f;
}

/**
 * @brief Convert vehicle speed (PID 0x0D)
 * @param a Data byte A
 * @return Speed in km/h (0-255)
 */
static inline int obd2_calc_speed(uint8_t a) {
    return (int)a;
}

/**
 * @brief Convert intake air temperature (PID 0x0F)
 * @param a Data byte A
 * @return Temperature in °C (-40 to 215)
 */
static inline int obd2_calc_intake_temp(uint8_t a) {
    return (int)a - 40;
}

/**
 * @brief Convert MAF air flow rate (PID 0x10)
 * @param a Data byte A (high byte)
 * @param b Data byte B (low byte)
 * @return Flow rate in g/s (0-655.35)
 */
static inline float obd2_calc_maf(uint8_t a, uint8_t b) {
    return ((float)a * 256.0f + (float)b) / 100.0f;
}

/**
 * @brief Convert throttle position (PID 0x11)
 * @param a Data byte A
 * @return Position percentage (0-100%)
 */
static inline float obd2_calc_throttle(uint8_t a) {
    return (float)a * 100.0f / 255.0f;
}

/**
 * @brief Convert fuel tank level (PID 0x2F)
 * @param a Data byte A
 * @return Fuel level percentage (0-100%)
 */
static inline float obd2_calc_fuel_level(uint8_t a) {
    return (float)a * 100.0f / 255.0f;
}

/**
 * @brief Convert ambient temperature (PID 0x46)
 * @param a Data byte A
 * @return Temperature in °C (-40 to 215)
 */
static inline int obd2_calc_ambient_temp(uint8_t a) {
    return (int)a - 40;
}

/**
 * @brief Convert oil temperature (PID 0x5C)
 * @param a Data byte A
 * @return Temperature in °C (-40 to 210)
 */
static inline int obd2_calc_oil_temp(uint8_t a) {
    return (int)a - 40;
}

#endif /* VTU_OBD2_PIDS_H */