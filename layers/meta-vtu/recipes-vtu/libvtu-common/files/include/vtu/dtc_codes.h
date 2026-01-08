/**
 * @file dtc_codes.h
 * @brief Diagnostic Trouble Code (DTC) definitions
 * 
 * DTCs follow SAE J2012 / ISO 15031-6 format: PXYYY
 * P = Powertrain, B = Body, C = Chassis, U = Network
 * X = 0 (generic SAE) or 1 (manufacturer specific)
 * YYY = Specific fault code
 */

#ifndef VTU_DTC_CODES_H
#define VTU_DTC_CODES_H

#include <stdint.h>

/*============================================================================
 * DTC Category Prefixes
 *===========================================================================*/

#define DTC_CAT_POWERTRAIN  'P'     /* Engine, transmission */
#define DTC_CAT_BODY        'B'     /* Body systems */
#define DTC_CAT_CHASSIS     'C'     /* ABS, suspension */
#define DTC_CAT_NETWORK     'U'     /* CAN, communication */

/*============================================================================
 * DTC Structure
 *===========================================================================*/

/**
 * @brief Decoded DTC information
 */
struct vtu_dtc {
    char     code[6];           /* DTC string (e.g., "P0300") */
    uint16_t raw;               /* Raw 2-byte DTC value */
    uint8_t  status;            /* DTC status byte */
    char     description[64];   /* Human-readable description */
};

/*============================================================================
 * Common Powertrain DTCs (P0xxx - P0xxx)
 * These are the most frequently encountered codes
 *===========================================================================*/

/* Fuel and Air Metering */
#define DTC_P0100_DESC  "Mass Air Flow Circuit Malfunction"
#define DTC_P0101_DESC  "Mass Air Flow Circuit Range/Performance"
#define DTC_P0102_DESC  "Mass Air Flow Circuit Low Input"
#define DTC_P0103_DESC  "Mass Air Flow Circuit High Input"
#define DTC_P0106_DESC  "MAP/Barometric Pressure Circuit Range/Performance"
#define DTC_P0107_DESC  "MAP/Barometric Pressure Circuit Low Input"
#define DTC_P0108_DESC  "MAP/Barometric Pressure Circuit High Input"
#define DTC_P0110_DESC  "Intake Air Temperature Circuit Malfunction"
#define DTC_P0115_DESC  "Engine Coolant Temperature Circuit Malfunction"
#define DTC_P0116_DESC  "Engine Coolant Temperature Circuit Range/Performance"
#define DTC_P0117_DESC  "Engine Coolant Temperature Circuit Low Input"
#define DTC_P0118_DESC  "Engine Coolant Temperature Circuit High Input"
#define DTC_P0120_DESC  "Throttle Position Sensor Circuit Malfunction"
#define DTC_P0121_DESC  "Throttle Position Sensor Circuit Range/Performance"
#define DTC_P0122_DESC  "Throttle Position Sensor Circuit Low Input"
#define DTC_P0123_DESC  "Throttle Position Sensor Circuit High Input"
#define DTC_P0130_DESC  "O2 Sensor Circuit Malfunction (Bank 1 Sensor 1)"
#define DTC_P0131_DESC  "O2 Sensor Circuit Low Voltage (Bank 1 Sensor 1)"
#define DTC_P0132_DESC  "O2 Sensor Circuit High Voltage (Bank 1 Sensor 1)"
#define DTC_P0133_DESC  "O2 Sensor Circuit Slow Response (Bank 1 Sensor 1)"
#define DTC_P0134_DESC  "O2 Sensor Circuit No Activity Detected (Bank 1 Sensor 1)"

/* Fuel System */
#define DTC_P0171_DESC  "System Too Lean (Bank 1)"
#define DTC_P0172_DESC  "System Too Rich (Bank 1)"
#define DTC_P0174_DESC  "System Too Lean (Bank 2)"
#define DTC_P0175_DESC  "System Too Rich (Bank 2)"

/* Ignition System / Misfire */
#define DTC_P0300_DESC  "Random/Multiple Cylinder Misfire Detected"
#define DTC_P0301_DESC  "Cylinder 1 Misfire Detected"
#define DTC_P0302_DESC  "Cylinder 2 Misfire Detected"
#define DTC_P0303_DESC  "Cylinder 3 Misfire Detected"
#define DTC_P0304_DESC  "Cylinder 4 Misfire Detected"
#define DTC_P0305_DESC  "Cylinder 5 Misfire Detected"
#define DTC_P0306_DESC  "Cylinder 6 Misfire Detected"
#define DTC_P0307_DESC  "Cylinder 7 Misfire Detected"
#define DTC_P0308_DESC  "Cylinder 8 Misfire Detected"

/* Auxiliary Emission Controls */
#define DTC_P0400_DESC  "Exhaust Gas Recirculation Flow Malfunction"
#define DTC_P0401_DESC  "Exhaust Gas Recirculation Flow Insufficient Detected"
#define DTC_P0402_DESC  "Exhaust Gas Recirculation Flow Excessive Detected"
#define DTC_P0420_DESC  "Catalyst System Efficiency Below Threshold (Bank 1)"
#define DTC_P0430_DESC  "Catalyst System Efficiency Below Threshold (Bank 2)"
#define DTC_P0440_DESC  "Evaporative Emission Control System Malfunction"
#define DTC_P0442_DESC  "Evaporative Emission Control System Leak Detected (small leak)"
#define DTC_P0446_DESC  "Evaporative Emission Control System Vent Control Circuit"
#define DTC_P0455_DESC  "Evaporative Emission Control System Leak Detected (large leak)"

/* Vehicle Speed / Idle Control */
#define DTC_P0500_DESC  "Vehicle Speed Sensor Malfunction"
#define DTC_P0505_DESC  "Idle Control System Malfunction"
#define DTC_P0506_DESC  "Idle Control System RPM Lower Than Expected"
#define DTC_P0507_DESC  "Idle Control System RPM Higher Than Expected"

/* Transmission */
#define DTC_P0700_DESC  "Transmission Control System Malfunction"
#define DTC_P0715_DESC  "Input/Turbine Speed Sensor Circuit Malfunction"
#define DTC_P0720_DESC  "Output Speed Sensor Circuit Malfunction"
#define DTC_P0730_DESC  "Incorrect Gear Ratio"
#define DTC_P0731_DESC  "Gear 1 Incorrect Ratio"
#define DTC_P0732_DESC  "Gear 2 Incorrect Ratio"
#define DTC_P0733_DESC  "Gear 3 Incorrect Ratio"
#define DTC_P0734_DESC  "Gear 4 Incorrect Ratio"

/*============================================================================
 * DTC Decoding Functions
 *===========================================================================*/

/**
 * @brief Decode raw DTC bytes to string format
 * @param byte1 First DTC byte
 * @param byte2 Second DTC byte
 * @param out Output buffer (minimum 6 bytes)
 * 
 * Raw format: 2 bytes where:
 *   Bits 15-14: Category (00=P, 01=C, 10=B, 11=U)
 *   Bits 13-12: First digit
 *   Bits 11-8:  Second digit (BCD)
 *   Bits 7-4:   Third digit (BCD)
 *   Bits 3-0:   Fourth digit (BCD)
 */
static inline void dtc_decode(uint8_t byte1, uint8_t byte2, char *out) {
    static const char categories[] = {'P', 'C', 'B', 'U'};
    
    out[0] = categories[(byte1 >> 6) & 0x03];
    out[1] = '0' + ((byte1 >> 4) & 0x03);
    out[2] = '0' + (byte1 & 0x0F);
    out[3] = '0' + ((byte2 >> 4) & 0x0F);
    out[4] = '0' + (byte2 & 0x0F);
    out[5] = '\0';
}

/**
 * @brief Encode DTC string to raw bytes
 * @param dtc DTC string (e.g., "P0300")
 * @param byte1 Output first byte
 * @param byte2 Output second byte
 * @return 0 on success, -1 on invalid input
 */
static inline int dtc_encode(const char *dtc, uint8_t *byte1, uint8_t *byte2) {
    uint8_t cat;
    
    switch (dtc[0]) {
        case 'P': case 'p': cat = 0; break;
        case 'C': case 'c': cat = 1; break;
        case 'B': case 'b': cat = 2; break;
        case 'U': case 'u': cat = 3; break;
        default: return -1;
    }
    
    *byte1 = (cat << 6) | ((dtc[1] - '0') << 4) | (dtc[2] - '0');
    *byte2 = ((dtc[3] - '0') << 4) | (dtc[4] - '0');
    return 0;
}

#endif /* VTU_DTC_CODES_H */