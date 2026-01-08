/**
 * @file can_defs.h
 * @brief CAN Bus message definitions for Vehicle Telemetry Unit
 * 
 * Defines CAN frame IDs, signal layouts, and conversion macros
 * following automotive industry conventions.
 */

#ifndef VTU_CAN_DEFS_H
#define VTU_CAN_DEFS_H

#include <stdint.h>

/*============================================================================
 * CAN Frame IDs - Broadcast Messages (ECU → Bus)
 *===========================================================================*/

/* Engine ECU messages */
#define CAN_ID_ENGINE_DATA_1    0x100   /* 10ms cycle: RPM, coolant, throttle, MAF */
#define CAN_ID_ENGINE_DATA_2    0x101   /* 100ms cycle: load, intake temp, timing */

/* Transmission ECU messages */
#define CAN_ID_TRANS_DATA       0x200   /* 50ms cycle: gear, fluid temp, speed */

/* Body Control Module messages */
#define CAN_ID_BCM_DATA         0x300   /* 100ms cycle: fuel level, odometer */

/* ABS/ESP messages */
#define CAN_ID_ABS_WHEEL_SPEED  0x400   /* 20ms cycle: wheel speeds */

/*============================================================================
 * OBD-II Diagnostic CAN IDs (ISO 15765-4)
 *===========================================================================*/

#define CAN_ID_OBD_BROADCAST    0x7DF   /* Tester broadcast request */
#define CAN_ID_OBD_ECU_ENGINE   0x7E0   /* Request to Engine ECU */
#define CAN_ID_OBD_ECU_TRANS    0x7E1   /* Request to Transmission ECU */
#define CAN_ID_OBD_RESP_ENGINE  0x7E8   /* Response from Engine ECU */
#define CAN_ID_OBD_RESP_TRANS   0x7E9   /* Response from Transmission ECU */

/*============================================================================
 * CAN Frame Structure
 *===========================================================================*/

/**
 * @brief Standard CAN frame (CAN 2.0B)
 */
struct vtu_can_frame {
    uint32_t can_id;        /* 11-bit standard ID (or 29-bit extended) */
    uint8_t  dlc;           /* Data Length Code (0-8) */
    uint8_t  data[8];       /* Payload */
    uint64_t timestamp_us;  /* Microsecond timestamp */
};

/*============================================================================
 * Signal Extraction Macros
 * 
 * CAN signals are packed into bytes. These macros extract them.
 * Automotive typically uses big-endian (Motorola) byte order.
 *===========================================================================*/

/* Extract 16-bit value from bytes (big-endian) */
#define CAN_GET_U16_BE(data, offset) \
    ((uint16_t)((data)[(offset)] << 8) | (data)[(offset) + 1])

/* Extract 16-bit value from bytes (little-endian) */
#define CAN_GET_U16_LE(data, offset) \
    ((uint16_t)((data)[(offset) + 1] << 8) | (data)[(offset)])

/* Extract 8-bit value */
#define CAN_GET_U8(data, offset) ((uint8_t)(data)[(offset)])

/*============================================================================
 * ENGINE_DATA_1 (0x100) Signal Layout - 10ms cycle
 * 
 * Byte 0-1: Engine RPM (0.25 rpm/bit) - range 0-16383.75 rpm
 * Byte 2:   Coolant temp (°C + 40 offset) - range -40 to 215°C
 * Byte 3:   Throttle position (0.392%/bit) - range 0-100%
 * Byte 4-5: MAF (0.01 g/s per bit) - range 0-655.35 g/s
 * Byte 6:   Reserved
 * Byte 7:   Reserved
 *===========================================================================*/

#define ENGINE1_RPM_FACTOR          0.25f
#define ENGINE1_COOLANT_OFFSET      40
#define ENGINE1_THROTTLE_FACTOR     0.392157f  /* 100/255 */
#define ENGINE1_MAF_FACTOR          0.01f

/* Signal extraction for ENGINE_DATA_1 */
#define ENGINE1_GET_RPM(data)       (CAN_GET_U16_BE(data, 0) * ENGINE1_RPM_FACTOR)
#define ENGINE1_GET_COOLANT(data)   (CAN_GET_U8(data, 2) - ENGINE1_COOLANT_OFFSET)
#define ENGINE1_GET_THROTTLE(data)  (CAN_GET_U8(data, 3) * ENGINE1_THROTTLE_FACTOR)
#define ENGINE1_GET_MAF(data)       (CAN_GET_U16_BE(data, 4) * ENGINE1_MAF_FACTOR)

/*============================================================================
 * TRANS_DATA (0x200) Signal Layout - 50ms cycle
 * 
 * Byte 0:   Current gear (0=N, 1-6=gear, 7=R)
 * Byte 1:   Transmission fluid temp (°C + 40 offset)
 * Byte 2-3: Output shaft speed (rpm)
 * Byte 4-7: Reserved
 *===========================================================================*/

#define TRANS_GEAR_NEUTRAL  0
#define TRANS_GEAR_REVERSE  7
#define TRANS_TEMP_OFFSET   40

#define TRANS_GET_GEAR(data)      CAN_GET_U8(data, 0)
#define TRANS_GET_FLUID_TEMP(data) (CAN_GET_U8(data, 1) - TRANS_TEMP_OFFSET)
#define TRANS_GET_OUTPUT_RPM(data) CAN_GET_U16_BE(data, 2)

#endif /* VTU_CAN_DEFS_H */