/*
 * VTU OBD-II Gateway
 * 
 * Listens for OBD-II diagnostic requests and responds with simulated data.
 * This bridges standard OBD-II tools with our virtual CAN bus.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <vtu/can_defs.h>
#include <vtu/obd2_pids.h>

/* OBD-II CAN IDs */
#define OBD2_REQUEST_BROADCAST  0x7DF   /* Broadcast request (all ECUs) */
#define OBD2_REQUEST_ECU1       0x7E0   /* Direct to ECU 1 */
#define OBD2_RESPONSE_ECU1      0x7E8   /* Response from ECU 1 */

/* OBD-II Modes */
#define OBD2_MODE_CURRENT_DATA  0x01
#define OBD2_MODE_FREEZE_FRAME  0x02
#define OBD2_MODE_DTC_READ      0x03
#define OBD2_MODE_DTC_CLEAR     0x04
#define OBD2_MODE_VEHICLE_INFO  0x09

/* Simulated vehicle state */
static struct {
    uint16_t rpm;           /* Engine RPM */
    uint8_t  speed;         /* Vehicle speed km/h */
    int8_t   coolant_temp;  /* Coolant temp (offset by -40) */
    uint8_t  throttle;      /* Throttle position % */
    uint8_t  fuel_level;    /* Fuel level % */
    uint16_t engine_load;   /* Calculated engine load */
    uint16_t maf;           /* MAF air flow rate */
    uint8_t  intake_temp;   /* Intake air temp (offset by -40) */
} vehicle_state = {
    .rpm = 850,             /* Idle RPM */
    .speed = 0,
    .coolant_temp = 90,     /* Normal operating temp */
    .throttle = 15,
    .fuel_level = 75,
    .engine_load = 25,
    .maf = 10,
    .intake_temp = 25,
};

static volatile int running = 1;
static int can_socket = -1;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

/* Update simulated values with slight variations */
static void update_simulation(void) {
    static int tick = 0;
    tick++;
    
    /* Simulate idle with small variations */
    vehicle_state.rpm = 800 + (rand() % 100);
    vehicle_state.throttle = 12 + (rand() % 6);
    vehicle_state.engine_load = 20 + (rand() % 10);
    
    /* Slowly varying values */
    if (tick % 10 == 0) {
        if (vehicle_state.coolant_temp < 95)
            vehicle_state.coolant_temp++;
    }
}

/* Build OBD-II response for Mode 01 (Current Data) */
static int build_mode01_response(uint8_t pid, struct can_frame *response) {
    response->can_id = OBD2_RESPONSE_ECU1;
    memset(response->data, 0, 8);
    
    switch (pid) {
        case OBD2_PID_ENGINE_LOAD:
            /* A * 100 / 255 = % */
            response->data[0] = 3;  /* Length */
            response->data[1] = 0x41;
            response->data[2] = pid;
            response->data[3] = (vehicle_state.engine_load * 255) / 100;
            response->len = 8;
            break;
            
        case OBD2_PID_COOLANT_TEMP:
            /* A - 40 = °C */
            response->data[0] = 3;
            response->data[1] = 0x41;
            response->data[2] = pid;
            response->data[3] = vehicle_state.coolant_temp + 40;
            response->len = 8;
            break;
            
        case OBD2_PID_ENGINE_RPM:
            /* ((A * 256) + B) / 4 = RPM */
            {
                uint16_t rpm_encoded = vehicle_state.rpm * 4;
                response->data[0] = 4;
                response->data[1] = 0x41;
                response->data[2] = pid;
                response->data[3] = (rpm_encoded >> 8) & 0xFF;
                response->data[4] = rpm_encoded & 0xFF;
                response->len = 8;
            }
            break;
            
        case OBD2_PID_VEHICLE_SPEED:
            /* A = km/h */
            response->data[0] = 3;
            response->data[1] = 0x41;
            response->data[2] = pid;
            response->data[3] = vehicle_state.speed;
            response->len = 8;
            break;
            
        case OBD2_PID_INTAKE_TEMP:
            /* A - 40 = °C */
            response->data[0] = 3;
            response->data[1] = 0x41;
            response->data[2] = pid;
            response->data[3] = vehicle_state.intake_temp + 40;
            response->len = 8;
            break;
            
        case OBD2_PID_MAF:
            /* ((A * 256) + B) / 100 = g/s */
            {
                uint16_t maf_encoded = vehicle_state.maf * 100;
                response->data[0] = 4;
                response->data[1] = 0x41;
                response->data[2] = pid;
                response->data[3] = (maf_encoded >> 8) & 0xFF;
                response->data[4] = maf_encoded & 0xFF;
                response->len = 8;
            }
            break;
            
        case OBD2_PID_THROTTLE_POS:
            /* A * 100 / 255 = % */
            response->data[0] = 3;
            response->data[1] = 0x41;
            response->data[2] = pid;
            response->data[3] = (vehicle_state.throttle * 255) / 100;
            response->len = 8;
            break;
            
        case OBD2_PID_FUEL_LEVEL:
            /* A * 100 / 255 = % */
            response->data[0] = 3;
            response->data[1] = 0x41;
            response->data[2] = pid;
            response->data[3] = (vehicle_state.fuel_level * 255) / 100;
            response->len = 8;
            break;
            
        case OBD2_PID_SUPPORTED_01_20:
            /* Bitmap of supported PIDs 01-20 */
            response->data[0] = 6;
            response->data[1] = 0x41;
            response->data[2] = pid;
            /* We support: 04,05,0C,0D,0F,10,11,2F */
            response->data[3] = 0x18;  /* PIDs 04,05 */
            response->data[4] = 0x38;  /* PIDs 0C,0D,0F */
            response->data[5] = 0x80;  /* PID 10 */
            response->data[6] = 0x01;  /* PIDs 11, link to 21-40 */
            response->len = 8;
            break;
            
        case OBD2_PID_SUPPORTED_21_40:
            /* Bitmap of supported PIDs 21-40 */
            response->data[0] = 6;
            response->data[1] = 0x41;
            response->data[2] = pid;
            response->data[3] = 0x00;
            response->data[4] = 0x00;
            response->data[5] = 0x40;  /* PID 2F (fuel level) */
            response->data[6] = 0x00;
            response->len = 8;
            break;
            
        default:
            /* Unsupported PID - no response */
            return -1;
    }
    
    return 0;
}

/* Process incoming OBD-II request */
static void process_obd2_request(struct can_frame *request) {
    uint8_t length = request->data[0];
    uint8_t mode = request->data[1];
    uint8_t pid = request->data[2];
    struct can_frame response;
    
    printf("[OBDGW] Request: Mode=%02X PID=%02X\n", mode, pid);
    
    if (length < 2) {
        printf("[OBDGW] Invalid request length\n");
        return;
    }
    
    switch (mode) {
        case OBD2_MODE_CURRENT_DATA:
            if (build_mode01_response(pid, &response) == 0) {
                if (write(can_socket, &response, sizeof(response)) < 0) {
                    perror("[OBDGW] Failed to send response");
                } else {
                    printf("[OBDGW] Response: %02X %02X %02X %02X %02X\n",
                           response.data[0], response.data[1], response.data[2],
                           response.data[3], response.data[4]);
                }
            } else {
                printf("[OBDGW] Unsupported PID: %02X\n", pid);
            }
            break;
            
        default:
            printf("[OBDGW] Unsupported mode: %02X\n", mode);
            break;
    }
}

static int setup_can_socket(const char *ifname) {
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_filter filter[2];
    
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("[OBDGW] Failed to create CAN socket");
        return -1;
    }
    
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("[OBDGW] Failed to get interface index");
        close(can_socket);
        return -1;
    }
    
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[OBDGW] Failed to bind CAN socket");
        close(can_socket);
        return -1;
    }
    
    /* Filter for OBD-II request IDs only */
    filter[0].can_id = OBD2_REQUEST_BROADCAST;
    filter[0].can_mask = CAN_SFF_MASK;
    filter[1].can_id = OBD2_REQUEST_ECU1;
    filter[1].can_mask = CAN_SFF_MASK;
    
    if (setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER, 
                   filter, sizeof(filter)) < 0) {
        perror("[OBDGW] Failed to set CAN filter");
        close(can_socket);
        return -1;
    }
    
    printf("[OBDGW] Listening on %s for OBD-II requests (7DF, 7E0)\n", ifname);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *can_if = "vcan0";
    struct can_frame frame;
    fd_set rdfs;
    struct timeval tv;
    
    if (argc > 1) {
        can_if = argv[1];
    }
    
    printf("VTU OBD-II Gateway v1.0\n");
    printf("=======================\n");
    
    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Seed random for simulation variations */
    srand(time(NULL));
    
    if (setup_can_socket(can_if) < 0) {
        return 1;
    }
    
    printf("[OBDGW] Ready to respond to OBD-II queries\n");
    printf("[OBDGW] Supported: Mode 01 PIDs 04,05,0C,0D,0F,10,11,2F\n\n");
    
    while (running) {
        FD_ZERO(&rdfs);
        FD_SET(can_socket, &rdfs);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int ret = select(can_socket + 1, &rdfs, NULL, NULL, &tv);
        
        if (ret < 0 && errno != EINTR) {
            perror("[OBDGW] select()");
            break;
        }
        
        if (ret > 0 && FD_ISSET(can_socket, &rdfs)) {
            ssize_t nbytes = read(can_socket, &frame, sizeof(frame));
            if (nbytes < 0) {
                perror("[OBDGW] read()");
                continue;
            }
            
            if (nbytes == sizeof(frame)) {
                process_obd2_request(&frame);
            }
        }
        
        /* Update simulation even when idle */
        update_simulation();
    }
    
    printf("\n[OBDGW] Shutting down...\n");
    close(can_socket);
    return 0;
}