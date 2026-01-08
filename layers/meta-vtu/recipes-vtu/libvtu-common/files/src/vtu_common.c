/**
 * @file vtu_common.c
 * @brief VTU Common Library implementation
 * 
 * Utility functions for CAN and OBD-II operations.
 */

#include <stdio.h>
#include <string.h>
#include "vtu/can_defs.h"
#include "vtu/obd2_pids.h"
#include "vtu/dtc_codes.h"

/**
 * @brief Get human-readable description for a DTC
 * @param dtc_code DTC string (e.g., "P0300")
 * @return Description string or "Unknown DTC"
 */
const char* vtu_get_dtc_description(const char *dtc_code) {
    /* Common DTCs lookup - in production this would be a hash table */
    if (strcmp(dtc_code, "P0100") == 0) return DTC_P0100_DESC;
    if (strcmp(dtc_code, "P0101") == 0) return DTC_P0101_DESC;
    if (strcmp(dtc_code, "P0102") == 0) return DTC_P0102_DESC;
    if (strcmp(dtc_code, "P0103") == 0) return DTC_P0103_DESC;
    if (strcmp(dtc_code, "P0115") == 0) return DTC_P0115_DESC;
    if (strcmp(dtc_code, "P0116") == 0) return DTC_P0116_DESC;
    if (strcmp(dtc_code, "P0117") == 0) return DTC_P0117_DESC;
    if (strcmp(dtc_code, "P0118") == 0) return DTC_P0118_DESC;
    if (strcmp(dtc_code, "P0120") == 0) return DTC_P0120_DESC;
    if (strcmp(dtc_code, "P0121") == 0) return DTC_P0121_DESC;
    if (strcmp(dtc_code, "P0171") == 0) return DTC_P0171_DESC;
    if (strcmp(dtc_code, "P0172") == 0) return DTC_P0172_DESC;
    if (strcmp(dtc_code, "P0300") == 0) return DTC_P0300_DESC;
    if (strcmp(dtc_code, "P0301") == 0) return DTC_P0301_DESC;
    if (strcmp(dtc_code, "P0302") == 0) return DTC_P0302_DESC;
    if (strcmp(dtc_code, "P0303") == 0) return DTC_P0303_DESC;
    if (strcmp(dtc_code, "P0304") == 0) return DTC_P0304_DESC;
    if (strcmp(dtc_code, "P0420") == 0) return DTC_P0420_DESC;
    if (strcmp(dtc_code, "P0430") == 0) return DTC_P0430_DESC;
    if (strcmp(dtc_code, "P0440") == 0) return DTC_P0440_DESC;
    if (strcmp(dtc_code, "P0442") == 0) return DTC_P0442_DESC;
    if (strcmp(dtc_code, "P0455") == 0) return DTC_P0455_DESC;
    if (strcmp(dtc_code, "P0500") == 0) return DTC_P0500_DESC;
    if (strcmp(dtc_code, "P0505") == 0) return DTC_P0505_DESC;
    if (strcmp(dtc_code, "P0700") == 0) return DTC_P0700_DESC;
    if (strcmp(dtc_code, "P0715") == 0) return DTC_P0715_DESC;
    if (strcmp(dtc_code, "P0720") == 0) return DTC_P0720_DESC;
    
    return "Unknown DTC";
}

/**
 * @brief Get PID name string
 * @param pid PID number
 * @return Name string or "Unknown PID"
 */
const char* vtu_get_pid_name(uint8_t pid) {
    switch (pid) {
        case OBD2_PID_ENGINE_LOAD:   return "Engine Load";
        case OBD2_PID_COOLANT_TEMP:  return "Coolant Temperature";
        case OBD2_PID_ENGINE_RPM:    return "Engine RPM";
        case OBD2_PID_VEHICLE_SPEED: return "Vehicle Speed";
        case OBD2_PID_INTAKE_TEMP:   return "Intake Air Temperature";
        case OBD2_PID_MAF:           return "MAF Air Flow Rate";
        case OBD2_PID_THROTTLE_POS:  return "Throttle Position";
        case OBD2_PID_FUEL_LEVEL:    return "Fuel Tank Level";
        case OBD2_PID_AMBIENT_TEMP:  return "Ambient Air Temperature";
        case OBD2_PID_OIL_TEMP:      return "Engine Oil Temperature";
        default:                      return "Unknown PID";
    }
}

/**
 * @brief Get PID units string
 * @param pid PID number
 * @return Units string
 */
const char* vtu_get_pid_units(uint8_t pid) {
    switch (pid) {
        case OBD2_PID_ENGINE_LOAD:   return "%";
        case OBD2_PID_COOLANT_TEMP:  return "°C";
        case OBD2_PID_ENGINE_RPM:    return "rpm";
        case OBD2_PID_VEHICLE_SPEED: return "km/h";
        case OBD2_PID_INTAKE_TEMP:   return "°C";
        case OBD2_PID_MAF:           return "g/s";
        case OBD2_PID_THROTTLE_POS:  return "%";
        case OBD2_PID_FUEL_LEVEL:    return "%";
        case OBD2_PID_AMBIENT_TEMP:  return "°C";
        case OBD2_PID_OIL_TEMP:      return "°C";
        default:                      return "";
    }
}

/**
 * @brief Library version
 */
const char* vtu_common_version(void) {
    return "1.0.0";
}