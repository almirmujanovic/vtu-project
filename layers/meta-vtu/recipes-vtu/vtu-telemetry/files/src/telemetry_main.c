/*
 * VTU MQTT Telemetry Publisher
 * 
 * Reads vehicle data from CAN bus and publishes to MQTT broker.
 * Enables remote monitoring, cloud dashboards, and fleet management.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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

#include <MQTTClient.h>

/* Configuration */
#define DEFAULT_BROKER      "tcp://localhost:1883"
#define CLIENT_ID           "vtu-telemetry-001"
#define TOPIC_PREFIX        "vtu/vehicle001"
#define QOS                 1
#define TIMEOUT             10000L
#define PUBLISH_INTERVAL_MS 1000   /* Publish every second */

/* CAN IDs from ECU simulator */
#define CAN_ID_ENGINE   0x100
#define CAN_ID_THROTTLE 0x101
#define CAN_ID_SPEED    0x200
#define CAN_ID_FUEL     0x300

static volatile int running = 1;
static int can_socket = -1;
static MQTTClient mqtt_client;
static int mqtt_connected = 0;

/* Vehicle state decoded from CAN */
static struct {
    uint16_t rpm;
    int8_t   coolant_temp;
    uint8_t  engine_load;
    uint8_t  throttle;
    uint8_t  speed;
    uint32_t odometer;
    uint8_t  fuel_level;
    time_t   last_update;
} vehicle = {0};

static void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

/* Decode CAN frame and update vehicle state */
static void decode_can_frame(struct can_frame *frame) {
    switch (frame->can_id & CAN_SFF_MASK) {
        case CAN_ID_ENGINE:
            vehicle.rpm = (frame->data[0] << 8) | frame->data[1];
            vehicle.rpm /= 4;  /* Convert from 0.25 RPM units */
            vehicle.coolant_temp = frame->data[2] - 40;
            vehicle.engine_load = frame->data[3];
            vehicle.last_update = time(NULL);
            break;
            
        case CAN_ID_THROTTLE:
            vehicle.throttle = (frame->data[0] * 100) / 255;
            break;
            
        case CAN_ID_SPEED:
            vehicle.speed = frame->data[0];
            vehicle.odometer = (frame->data[2] << 16) | 
                              (frame->data[3] << 8) | 
                              frame->data[4];
            break;
            
        case CAN_ID_FUEL:
            vehicle.fuel_level = (frame->data[0] * 100) / 255;
            break;
    }
}

/* Publish a single value to MQTT */
static void publish_value(const char *subtopic, const char *value) {
    char topic[128];
    MQTTClient_message msg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    
    snprintf(topic, sizeof(topic), "%s/%s", TOPIC_PREFIX, subtopic);
    
    msg.payload = (void *)value;
    msg.payloadlen = strlen(value);
    msg.qos = QOS;
    msg.retained = 1;  /* Retain last value */
    
    int rc = MQTTClient_publishMessage(mqtt_client, topic, &msg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "[TELEM] Publish failed: %d\n", rc);
    }
}

/* Publish all vehicle data as JSON status */
static void publish_status(void) {
    char json[512];
    char value[32];
    
    if (!mqtt_connected) {
        return;
    }
    
    /* Publish individual values */
    snprintf(value, sizeof(value), "%d", vehicle.rpm);
    publish_value("engine/rpm", value);
    
    snprintf(value, sizeof(value), "%d", vehicle.coolant_temp);
    publish_value("engine/coolant", value);
    
    snprintf(value, sizeof(value), "%d", vehicle.engine_load);
    publish_value("engine/load", value);
    
    snprintf(value, sizeof(value), "%d", vehicle.throttle);
    publish_value("engine/throttle", value);
    
    snprintf(value, sizeof(value), "%d", vehicle.speed);
    publish_value("speed", value);
    
    snprintf(value, sizeof(value), "%u", vehicle.odometer);
    publish_value("odometer", value);
    
    snprintf(value, sizeof(value), "%d", vehicle.fuel_level);
    publish_value("fuel/level", value);
    
    /* Publish combined JSON status */
    snprintf(json, sizeof(json),
        "{"
        "\"rpm\":%d,"
        "\"coolant\":%d,"
        "\"load\":%d,"
        "\"throttle\":%d,"
        "\"speed\":%d,"
        "\"odometer\":%u,"
        "\"fuel_level\":%d,"
        "\"timestamp\":%ld"
        "}",
        vehicle.rpm, vehicle.coolant_temp, vehicle.engine_load,
        vehicle.throttle, vehicle.speed, vehicle.odometer,
        vehicle.fuel_level, vehicle.last_update);
    
    publish_value("status", json);
    
    printf("[TELEM] Published: RPM=%d Speed=%d Coolant=%d°C Fuel=%d%%\n",
           vehicle.rpm, vehicle.speed, vehicle.coolant_temp, vehicle.fuel_level);
}

static int setup_mqtt(const char *broker) {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    
    rc = MQTTClient_create(&mqtt_client, broker, CLIENT_ID,
                           MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "[TELEM] Failed to create MQTT client: %d\n", rc);
        return -1;
    }
    
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    
    printf("[TELEM] Connecting to MQTT broker: %s\n", broker);
    
    rc = MQTTClient_connect(mqtt_client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "[TELEM] Failed to connect to broker: %d\n", rc);
        fprintf(stderr, "[TELEM] Will retry in background...\n");
        return 0;  /* Don't fail, just run without MQTT */
    }
    
    printf("[TELEM] Connected to MQTT broker\n");
    mqtt_connected = 1;
    return 0;
}

static int setup_can_socket(const char *ifname) {
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_filter filters[4];
    
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
    
    /* Filter for vehicle data CAN IDs only */
    filters[0].can_id = CAN_ID_ENGINE;
    filters[0].can_mask = CAN_SFF_MASK;
    filters[1].can_id = CAN_ID_THROTTLE;
    filters[1].can_mask = CAN_SFF_MASK;
    filters[2].can_id = CAN_ID_SPEED;
    filters[2].can_mask = CAN_SFF_MASK;
    filters[3].can_id = CAN_ID_FUEL;
    filters[3].can_mask = CAN_SFF_MASK;
    
    setsockopt(can_socket, SOL_CAN_RAW, CAN_RAW_FILTER,
               filters, sizeof(filters));
    
    /* Set non-blocking for select() */
    struct timeval tv = {0, 100000};  /* 100ms timeout */
    setsockopt(can_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    printf("[TELEM] Listening on %s\n", ifname);
    return 0;
}

static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("Options:\n");
    printf("  -b BROKER   MQTT broker URL (default: %s)\n", DEFAULT_BROKER);
    printf("  -i IFACE    CAN interface (default: vcan0)\n");
    printf("  -h          Show this help\n");
}

int main(int argc, char *argv[]) {
    const char *broker = DEFAULT_BROKER;
    const char *can_if = "vcan0";
    struct can_frame frame;
    time_t last_publish = 0;
    int opt;
    
    while ((opt = getopt(argc, argv, "b:i:h")) != -1) {
        switch (opt) {
            case 'b':
                broker = optarg;
                break;
            case 'i':
                can_if = optarg;
                break;
            case 'h':
            default:
                print_usage(argv[0]);
                return opt == 'h' ? 0 : 1;
        }
    }
    
    printf("VTU MQTT Telemetry v1.0\n");
    printf("=======================\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    if (setup_can_socket(can_if) < 0) {
        return 1;
    }
    
    setup_mqtt(broker);  /* Don't fail if broker unavailable */
    
    printf("[TELEM] Publishing to topic prefix: %s\n", TOPIC_PREFIX);
    printf("[TELEM] Publish interval: %d ms\n\n", PUBLISH_INTERVAL_MS);
    
    while (running) {
        /* Read CAN frames */
        ssize_t nbytes = read(can_socket, &frame, sizeof(frame));
        if (nbytes == sizeof(frame)) {
            decode_can_frame(&frame);
        }
        
        /* Publish at regular intervals */
        time_t now = time(NULL);
        if (now - last_publish >= (PUBLISH_INTERVAL_MS / 1000)) {
            publish_status();
            last_publish = now;
        }
        
        /* Try to reconnect if disconnected */
        if (!mqtt_connected && (now % 10 == 0)) {
            MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
            conn_opts.keepAliveInterval = 20;
            conn_opts.cleansession = 1;
            if (MQTTClient_connect(mqtt_client, &conn_opts) == MQTTCLIENT_SUCCESS) {
                printf("[TELEM] Reconnected to MQTT broker\n");
                mqtt_connected = 1;
            }
        }
    }
    
    printf("\n[TELEM] Shutting down...\n");
    
    if (mqtt_connected) {
        MQTTClient_disconnect(mqtt_client, TIMEOUT);
    }
    MQTTClient_destroy(&mqtt_client);
    close(can_socket);
    
    return 0;
}