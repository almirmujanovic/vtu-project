#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

static volatile int running = 1;
static struct {
    int rpm, speed, throttle, fuel, temp, load;
} v = {0};

static void handler(int s) { (void)s; running = 0; }

int main(int argc, char **argv) {
    const char *ifname = argc > 1 ? argv[1] : "vcan0";
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame f;
    int s;
    
    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    
    s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) { perror("socket"); return 1; }
    
    strcpy(ifr.ifr_name, ifname);
    ioctl(s, SIOCGIFINDEX, &ifr);
    
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(s, (struct sockaddr *)&addr, sizeof(addr));
    
    printf("VTU Console on %s - Ctrl+C to quit\n\n", ifname);
    
    while (running) {
        int n = read(s, &f, sizeof(f));
        if (n > 0) {
            switch (f.can_id & 0x7FF) {
                case 0x100:
                    v.rpm = ((f.data[0] << 8) | f.data[1]) / 4;
                    v.temp = f.data[2] - 40;
                    v.load = f.data[3];
                    break;
                case 0x101:
                    v.throttle = (f.data[0] * 100) / 255;
                    break;
                case 0x200:
                    v.speed = f.data[0];
                    break;
                case 0x300:
                    v.fuel = (f.data[0] * 100) / 255;
                    break;
            }
            printf("RPM:%5d  SPEED:%3d km/h  THROTTLE:%3d%%  FUEL:%3d%%  TEMP:%3dC  LOAD:%3d%%\n",
                   v.rpm, v.speed, v.throttle, v.fuel, v.temp, v.load);
        }
    }
    
    close(s);
    printf("\nDone.\n");
    return 0;
}
