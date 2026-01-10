/*
 * VTU CAN Bus Data Logger
 * 
 * Captures and logs all CAN bus traffic with timestamps.
 * Supports rotating log files and provides statistics.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#define LOG_DIR "/var/log/vtu"
#define MAX_LOG_SIZE (10 * 1024 * 1024)  /* 10 MB per file */
#define MAX_LOG_FILES 5                   /* Keep last 5 files */

static volatile int running = 1;
static int can_socket = -1;
static FILE *log_file = NULL;
static unsigned long frame_count = 0;
static unsigned long bytes_logged = 0;
static int current_file_num = 0;

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

/* Get current timestamp as string with microsecond precision */
static void get_timestamp(char *buf, size_t len) {
    struct timeval tv;
    struct tm *tm;
    
    gettimeofday(&tv, NULL);
    tm = localtime(&tv.tv_sec);
    
    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
             tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec,
             tv.tv_usec);
}

/* Rotate log files: delete oldest if we have too many */
static void rotate_logs(void) {
    char old_path[256], new_path[256];
    
    /* Remove oldest file if we've hit the limit */
    snprintf(old_path, sizeof(old_path), "%s/can-%d.log", 
             LOG_DIR, current_file_num - MAX_LOG_FILES);
    unlink(old_path);  /* Ignore errors if file doesn't exist */
}

/* Open a new log file */
static int open_log_file(void) {
    char filepath[256];
    char timestamp[64];
    
    if (log_file) {
        fprintf(log_file, "\n--- Log file closed ---\n");
        fclose(log_file);
    }
    
    current_file_num++;
    snprintf(filepath, sizeof(filepath), "%s/can-%d.log", 
             LOG_DIR, current_file_num);
    
    log_file = fopen(filepath, "w");
    if (!log_file) {
        perror("Failed to open log file");
        return -1;
    }
    
    get_timestamp(timestamp, sizeof(timestamp));
    fprintf(log_file, "=== VTU CAN Bus Log ===\n");
    fprintf(log_file, "Started: %s\n", timestamp);
    fprintf(log_file, "Format: TIMESTAMP CAN_ID [DLC] DATA\n");
    fprintf(log_file, "========================\n\n");
    fflush(log_file);
    
    bytes_logged = 0;
    rotate_logs();
    
    printf("[LOGGER] Opened log file: %s\n", filepath);
    return 0;
}

/* Log a CAN frame */
static void log_frame(struct can_frame *frame) {
    char timestamp[64];
    
    if (!log_file) {
        return;
    }
    
    /* Check if we need to rotate */
    if (bytes_logged >= MAX_LOG_SIZE) {
        open_log_file();
    }
    
    get_timestamp(timestamp, sizeof(timestamp));
    
    /* Write timestamp and CAN ID */
    int written = fprintf(log_file, "%s  %03X  [%d] ", 
                         timestamp, frame->can_id & CAN_SFF_MASK, 
                         frame->len);
    
    /* Write data bytes */
    for (int i = 0; i < frame->len; i++) {
        written += fprintf(log_file, " %02X", frame->data[i]);
    }
    
    written += fprintf(log_file, "\n");
    
    bytes_logged += written;
    frame_count++;
    
    /* Flush every 100 frames to ensure data is written */
    if (frame_count % 100 == 0) {
        fflush(log_file);
    }
}

/* Print statistics */
static void print_stats(void) {
    printf("\n[LOGGER] Statistics:\n");
    printf("  Frames logged: %lu\n", frame_count);
    printf("  Bytes written: %lu\n", bytes_logged);
    printf("  Current file:  %d\n", current_file_num);
}

static int setup_can_socket(const char *ifname) {
    struct sockaddr_can addr;
    struct ifreq ifr;
    
    can_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket < 0) {
        perror("Failed to create CAN socket");
        return -1;
    }
    
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(can_socket, SIOCGIFINDEX, &ifr) < 0) {
        perror("Failed to get interface index");
        close(can_socket);
        return -1;
    }
    
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(can_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind CAN socket");
        close(can_socket);
        return -1;
    }
    
    /* Set timestamp option for accurate logging */
    int timestamp_on = 1;
    setsockopt(can_socket, SOL_SOCKET, SO_TIMESTAMP, 
               &timestamp_on, sizeof(timestamp_on));
    
    printf("[LOGGER] Listening on %s\n", ifname);
    return 0;
}

int main(int argc, char *argv[]) {
    const char *can_if = "vcan0";
    struct can_frame frame;
    struct timeval last_stat_time, now;
    
    if (argc > 1) {
        can_if = argv[1];
    }
    
    printf("VTU CAN Bus Logger v1.0\n");
    printf("=======================\n");
    
    /* Create log directory if it doesn't exist */
    mkdir(LOG_DIR, 0755);
    
    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (setup_can_socket(can_if) < 0) {
        return 1;
    }
    
    if (open_log_file() < 0) {
        return 1;
    }
    
    printf("[LOGGER] Logging to %s/\n", LOG_DIR);
    printf("[LOGGER] Max file size: %d MB\n", MAX_LOG_SIZE / (1024*1024));
    printf("[LOGGER] Keeping last %d files\n", MAX_LOG_FILES);
    printf("[LOGGER] Press Ctrl+C to stop\n\n");
    
    gettimeofday(&last_stat_time, NULL);
    
    while (running) {
        ssize_t nbytes = read(can_socket, &frame, sizeof(frame));
        
        if (nbytes < 0) {
            if (errno == EINTR) {
                continue;  /* Interrupted by signal */
            }
            perror("CAN read error");
            break;
        }
        
        if (nbytes == sizeof(frame)) {
            log_frame(&frame);
        }
        
        /* Print statistics every 10 seconds */
        gettimeofday(&now, NULL);
        if (now.tv_sec - last_stat_time.tv_sec >= 10) {
            printf("[LOGGER] Logged %lu frames (%lu bytes)\n", 
                   frame_count, bytes_logged);
            last_stat_time = now;
        }
    }
    
    printf("\n[LOGGER] Shutting down...\n");
    print_stats();
    
    if (log_file) {
        char timestamp[64];
        get_timestamp(timestamp, sizeof(timestamp));
        fprintf(log_file, "\n--- Stopped: %s ---\n", timestamp);
        fclose(log_file);
    }
    
    close(can_socket);
    return 0;
}