/**
 * @file ecu_sim.c
 * @brief ECU Simulator - Generates simulated vehicle CAN data
 *
 * Simulates:
 * - Engine ECU (0x100, 0x101): RPM, coolant temp, throttle, MAF
 * - Transmission ECU (0x200): Gear, fluid temp
 * - Body Control Module (0x300): Fuel level, odometer
 * - OBD-II responses (0x7E8): Responds to diagnostic requests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "vtu/can_defs.h"
#include "vtu/obd2_pids.h"

/*============================================================================
 * Configuration
 *===========================================================================*/

#define CAN_INTERFACE   "vcan0"
#define ENGINE_CYCLE_MS     10      /* Engine data 1 broadcast rate */
#define ENGINE2_CYCLE_MS    100     /* Engine data 2 broadcast rate */
#define TRANS_CYCLE_MS      50      /* Transmission broadcast rate */
#define BCM_CYCLE_MS        100     /* Body control broadcast rate */

/*============================================================================
 * Simulated Vehicle State
 *===========================================================================*/

static struct {
    /* Engine */
    float rpm;              /* 0-8000 rpm */
    float coolant_temp;     /* -40 to 120 °C */
    float throttle;         /* 0-100% */
    float maf;              /* 0-200 g/s */
    float engine_load;      /* 0-100% */
    float intake_temp;      /* -40 to 60 °C */
    
    /* Transmission */
    int gear;               /* 0=N, 1-6, 7=R */
    float trans_temp;       /* -40 to 150 °C */
    
    /* Body */
    float fuel_level;       /* 0-100% */
    uint32_t odometer;      /* km */
    float vehicle_speed;    /* 0-255 km/h */
    
    /* Simulation */
    float sim_time;         /* Simulated time for varying values */
} vehicle = {
    .rpm = 800.0f,
    .coolant_temp = 85.0f,
    .throttle = 15.0f,
    .maf = 5.0f,
    .engine_load = 20.0f,
    .intake_temp = 25.0f,
    .gear = 0,
    .trans_temp = 60.0f,
    .fuel_level = 75.0f,
    .odometer = 45231,
    .vehicle_speed = 0.0f,
    .sim_time = 0.0f
};

/* Running flag for graceful shutdown */
static volatile sig_atomic_t running = 1;

/*============================================================================
 * Signal Handler
 *===========================================================================*/

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
    printf("\nShutting down ECU simulator...\n");
}

/*============================================================================
 * CAN Socket Functions
 *===========================================================================*/

/**
 * @brief Open a CAN socket
 * @param ifname Interface name (e.g., "vcan0")
 * @return Socket file descriptor or -1 on error
 */
static int can_socket_open(const char *ifname) {
    int sock;
    struct sockaddr_can addr;
    struct ifreq ifr;
    
    /* Create socket */
    sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    
    /* Get interface index */
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl SIOCGIFINDEX");
        close(sock);
        return -1;
    }
    
    /* Bind socket to interface */
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }
    
    return sock;
}

/**
 * @brief Send a CAN frame
 */
static int can_send(int sock, uint32_t id, const uint8_t *data, uint8_t len) {
    struct can_frame frame;
    
    memset(&frame, 0, sizeof(frame));
    frame.can_id = id;
    frame.can_dlc = len;
    memcpy(frame.data, data, len);
    
    if (write(sock, &frame, sizeof(frame)) != sizeof(frame)) {
        perror("write");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Receive a CAN frame (non-blocking)
 */
static int can_receive(int sock, struct can_frame *frame) {
    fd_set rdfs;
    struct timeval tv;
    int ret;
    
    FD_ZERO(&rdfs);
    FD_SET(sock, &rdfs);
    
    /* 1ms timeout for non-blocking behavior */
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    
    ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
    if (ret < 0) {
        perror("select");
        return -1;
    }
    if (ret == 0) {
        return 0;  /* Timeout, no data */
    }
    
    if (read(sock, frame, sizeof(*frame)) != sizeof(*frame)) {
        perror("read");
        return -1;
    }
    
    return 1;  /* Frame received */
}

/*============================================================================
 * Vehicle Simulation
 *===========================================================================*/

/**
 * @brief Update simulated vehicle values
 * 
 * Creates realistic-looking variations in sensor values.
 * Uses sine waves and random noise for natural behavior.
 */
static void update_simulation(float dt) {
    vehicle.sim_time += dt;
    
    /* Simulate driving pattern: idle -> accelerate -> cruise -> decelerate */
    float cycle = fmodf(vehicle.sim_time, 60.0f);  /* 60 second cycle */
    
    if (cycle < 10.0f) {
        /* Idle */
        vehicle.rpm = 800.0f + 50.0f * sinf(vehicle.sim_time * 2.0f);
        vehicle.throttle = 0.0f;
        vehicle.vehicle_speed = 0.0f;
        vehicle.gear = 0;
    } else if (cycle < 25.0f) {
        /* Accelerating */
        float accel_progress = (cycle - 10.0f) / 15.0f;
        vehicle.rpm = 800.0f + 4200.0f * accel_progress;
        vehicle.throttle = 30.0f + 50.0f * accel_progress;
        vehicle.vehicle_speed = 120.0f * accel_progress;
        vehicle.gear = 1 + (int)(accel_progress * 5);
        if (vehicle.gear > 6) vehicle.gear = 6;
    } else if (cycle < 45.0f) {
        /* Cruising */
        vehicle.rpm = 2500.0f + 200.0f * sinf(vehicle.sim_time * 0.5f);
        vehicle.throttle = 25.0f + 5.0f * sinf(vehicle.sim_time * 0.3f);
        vehicle.vehicle_speed = 100.0f + 10.0f * sinf(vehicle.sim_time * 0.2f);
        vehicle.gear = 6;
    } else {
        /* Decelerating */
        float decel_progress = (cycle - 45.0f) / 15.0f;
        vehicle.rpm = 2500.0f - 1700.0f * decel_progress;
        vehicle.throttle = 25.0f * (1.0f - decel_progress);
        vehicle.vehicle_speed = 100.0f * (1.0f - decel_progress);
        vehicle.gear = 6 - (int)(decel_progress * 5);
        if (vehicle.gear < 0) vehicle.gear = 0;
    }
    
    /* Engine load correlates with throttle */
    vehicle.engine_load = vehicle.throttle * 0.8f + 10.0f;
    
    /* MAF correlates with RPM and load */
    vehicle.maf = (vehicle.rpm / 1000.0f) * (vehicle.engine_load / 100.0f) * 15.0f;
    
    /* Temperatures vary slowly */
    vehicle.coolant_temp = 85.0f + 10.0f * sinf(vehicle.sim_time * 0.01f);
    vehicle.trans_temp = 70.0f + 20.0f * (vehicle.engine_load / 100.0f);
    vehicle.intake_temp = 25.0f + 5.0f * sinf(vehicle.sim_time * 0.05f);
    
    /* Fuel slowly decreases */
    vehicle.fuel_level = 75.0f - fmodf(vehicle.sim_time * 0.01f, 50.0f);
    
    /* Odometer increases with speed */
    vehicle.odometer += (uint32_t)(vehicle.vehicle_speed * dt / 3600.0f);
}

/*============================================================================
 * CAN Message Generation
 *===========================================================================*/

/**
 * @brief Build and send ENGINE_DATA_1 (0x100)
 */
static void send_engine_data_1(int sock) {
    uint8_t data[8] = {0};
    
    /* Bytes 0-1: RPM (0.25 rpm/bit, big-endian) */
    uint16_t rpm_raw = (uint16_t)(vehicle.rpm / 0.25f);
    data[0] = (rpm_raw >> 8) & 0xFF;
    data[1] = rpm_raw & 0xFF;
    
    /* Byte 2: Coolant temp (°C + 40 offset) */
    data[2] = (uint8_t)(vehicle.coolant_temp + 40.0f);
    
    /* Byte 3: Throttle position (0-255 = 0-100%) */
    data[3] = (uint8_t)(vehicle.throttle * 255.0f / 100.0f);
    
    /* Bytes 4-5: MAF (0.01 g/s per bit, big-endian) */
    uint16_t maf_raw = (uint16_t)(vehicle.maf / 0.01f);
    data[4] = (maf_raw >> 8) & 0xFF;
    data[5] = maf_raw & 0xFF;
    
    /* Byte 6: Engine load (0-255 = 0-100%) */
    data[6] = (uint8_t)(vehicle.engine_load * 255.0f / 100.0f);
    
    can_send(sock, CAN_ID_ENGINE_DATA_1, data, 8);
}

/**
 * @brief Build and send ENGINE_DATA_2 (0x101)
 */
static void send_engine_data_2(int sock) {
    uint8_t data[8] = {0};
    
    /* Byte 0: Intake air temp (°C + 40 offset) */
    data[0] = (uint8_t)(vehicle.intake_temp + 40.0f);
    
    /* Byte 1: Engine load (duplicate for compatibility) */
    data[1] = (uint8_t)(vehicle.engine_load * 255.0f / 100.0f);
    
    can_send(sock, CAN_ID_ENGINE_DATA_2, data, 8);
}

/**
 * @brief Build and send TRANS_DATA (0x200)
 */
static void send_trans_data(int sock) {
    uint8_t data[8] = {0};
    
    /* Byte 0: Current gear */
    data[0] = (uint8_t)vehicle.gear;
    
    /* Byte 1: Transmission fluid temp (°C + 40 offset) */
    data[1] = (uint8_t)(vehicle.trans_temp + 40.0f);
    
    /* Bytes 2-3: Vehicle speed (km/h, big-endian) */
    uint16_t speed_raw = (uint16_t)vehicle.vehicle_speed;
    data[2] = (speed_raw >> 8) & 0xFF;
    data[3] = speed_raw & 0xFF;
    
    can_send(sock, CAN_ID_TRANS_DATA, data, 8);
}

/**
 * @brief Build and send BCM_DATA (0x300)
 */
static void send_bcm_data(int sock) {
    uint8_t data[8] = {0};
    
    /* Byte 0: Fuel level (0-255 = 0-100%) */
    data[0] = (uint8_t)(vehicle.fuel_level * 255.0f / 100.0f);
    
    /* Bytes 1-4: Odometer (km, big-endian) */
    data[1] = (vehicle.odometer >> 24) & 0xFF;
    data[2] = (vehicle.odometer >> 16) & 0xFF;
    data[3] = (vehicle.odometer >> 8) & 0xFF;
    data[4] = vehicle.odometer & 0xFF;
    
    can_send(sock, CAN_ID_BCM_DATA, data, 8);
}

/*============================================================================
 * OBD-II Response Handler
 *===========================================================================*/

/**
 * @brief Handle OBD-II Mode 01 request
 */
static void handle_obd2_mode01(int sock, uint8_t pid) {
    uint8_t response[8] = {0};
    int len = 0;
    
    /* Response format: [num_bytes] [mode+0x40] [pid] [data...] */
    response[1] = OBD2_MODE_CURRENT_DATA + OBD2_RESPONSE_OFFSET;  /* 0x41 */
    response[2] = pid;
    
    switch (pid) {
        case OBD2_PID_SUPPORTED_01_20:
            /* Supported PIDs bitmap */
            response[0] = 6;
            response[3] = 0x18;  /* PIDs 04, 05 supported */
            response[4] = 0x3E;  /* PIDs 0C, 0D, 0F, 10, 11 supported */
            response[5] = 0x80;  /* PID 2F supported */
            response[6] = 0x00;
            len = 7;
            break;
            
        case OBD2_PID_ENGINE_LOAD:
            response[0] = 3;
            response[3] = (uint8_t)(vehicle.engine_load * 255.0f / 100.0f);
            len = 4;
            break;
            
        case OBD2_PID_COOLANT_TEMP:
            response[0] = 3;
            response[3] = (uint8_t)(vehicle.coolant_temp + 40.0f);
            len = 4;
            break;
            
        case OBD2_PID_ENGINE_RPM:
            response[0] = 4;
            {
                uint16_t rpm_raw = (uint16_t)(vehicle.rpm * 4.0f);
                response[3] = (rpm_raw >> 8) & 0xFF;
                response[4] = rpm_raw & 0xFF;
            }
            len = 5;
            break;
            
        case OBD2_PID_VEHICLE_SPEED:
            response[0] = 3;
            response[3] = (uint8_t)vehicle.vehicle_speed;
            len = 4;
            break;
            
        case OBD2_PID_INTAKE_TEMP:
            response[0] = 3;
            response[3] = (uint8_t)(vehicle.intake_temp + 40.0f);
            len = 4;
            break;
            
        case OBD2_PID_MAF:
            response[0] = 4;
            {
                uint16_t maf_raw = (uint16_t)(vehicle.maf * 100.0f);
                response[3] = (maf_raw >> 8) & 0xFF;
                response[4] = maf_raw & 0xFF;
            }
            len = 5;
            break;
            
        case OBD2_PID_THROTTLE_POS:
            response[0] = 3;
            response[3] = (uint8_t)(vehicle.throttle * 255.0f / 100.0f);
            len = 4;
            break;
            
        case OBD2_PID_FUEL_LEVEL:
            response[0] = 3;
            response[3] = (uint8_t)(vehicle.fuel_level * 255.0f / 100.0f);
            len = 4;
            break;
            
        default:
            /* Unsupported PID - send negative response */
            response[0] = 3;
            response[1] = 0x7F;  /* Negative response */
            response[2] = OBD2_MODE_CURRENT_DATA;
            response[3] = 0x12;  /* Sub-function not supported */
            len = 4;
            break;
    }
    
    if (len > 0) {
        can_send(sock, CAN_ID_OBD_RESP_ENGINE, response, len);
    }
}

/**
 * @brief Process incoming OBD-II request
 */
static void process_obd2_request(int sock, struct can_frame *frame) {
    uint8_t mode, pid;
    
    /* Check if it's an OBD-II request */
    if (frame->can_id != CAN_ID_OBD_BROADCAST && 
        frame->can_id != CAN_ID_OBD_ECU_ENGINE) {
        return;
    }
    
    /* Extract mode and PID */
    /* Frame format: [length] [mode] [pid] ... */
    if (frame->can_dlc < 2) return;
    
    mode = frame->data[1];
    pid = (frame->can_dlc >= 3) ? frame->data[2] : 0;
    
    switch (mode) {
        case OBD2_MODE_CURRENT_DATA:
            handle_obd2_mode01(sock, pid);
            break;
            
        /* TODO: Add Mode 03 (DTCs), Mode 09 (VIN) in future phases */
        
        default:
            break;
    }
}

/*============================================================================
 * Timing Utilities
 *===========================================================================*/

/**
 * @brief Get current time in milliseconds
 */
static uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/**
 * @brief Sleep for specified milliseconds
 */
static void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

/*============================================================================
 * Main
 *===========================================================================*/

int main(int argc, char *argv[]) {
    int sock;
    const char *ifname = CAN_INTERFACE;
    uint64_t last_engine1 = 0, last_engine2 = 0;
    uint64_t last_trans = 0, last_bcm = 0;
    uint64_t now;
    struct can_frame rx_frame;
    
    /* Parse command line */
    if (argc > 1) {
        ifname = argv[1];
    }
    
    printf("VTU ECU Simulator v1.0\n");
    printf("CAN Interface: %s\n", ifname);
    printf("Press Ctrl+C to stop\n\n");
    
    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Open CAN socket */
    sock = can_socket_open(ifname);
    if (sock < 0) {
        fprintf(stderr, "Failed to open CAN socket on %s\n", ifname);
        fprintf(stderr, "Make sure the interface exists: ip link show %s\n", ifname);
        return 1;
    }
    
    printf("ECU Simulator running. Broadcasting on %s\n", ifname);
    printf("  Engine Data 1 (0x100): every %d ms\n", ENGINE_CYCLE_MS);
    printf("  Engine Data 2 (0x101): every %d ms\n", ENGINE2_CYCLE_MS);
    printf("  Transmission  (0x200): every %d ms\n", TRANS_CYCLE_MS);
    printf("  Body Control  (0x300): every %d ms\n", BCM_CYCLE_MS);
    printf("  OBD-II responses on 0x7E8\n\n");
    
    /* Initialize timing */
    now = get_time_ms();
    last_engine1 = last_engine2 = last_trans = last_bcm = now;
    
    /* Main loop */
    while (running) {
        now = get_time_ms();
        
        /* Update simulation */
        update_simulation(0.001f);
        
        /* Send Engine Data 1 */
        if (now - last_engine1 >= ENGINE_CYCLE_MS) {
            send_engine_data_1(sock);
            last_engine1 = now;
        }
        
        /* Send Engine Data 2 */
        if (now - last_engine2 >= ENGINE2_CYCLE_MS) {
            send_engine_data_2(sock);
            last_engine2 = now;
        }
        
        /* Send Transmission Data */
        if (now - last_trans >= TRANS_CYCLE_MS) {
            send_trans_data(sock);
            last_trans = now;
        }
        
        /* Send Body Control Data */
        if (now - last_bcm >= BCM_CYCLE_MS) {
            send_bcm_data(sock);
            last_bcm = now;
        }
        
        /* Check for incoming OBD-II requests */
        if (can_receive(sock, &rx_frame) > 0) {
            process_obd2_request(sock, &rx_frame);
        }
        
        /* Small sleep to prevent busy-waiting */
        sleep_ms(1);
    }
    
    close(sock);
    printf("ECU Simulator stopped.\n");
    
    return 0;
}