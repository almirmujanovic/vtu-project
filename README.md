# Vehicle Telemetry Unit (VTU)

A complete embedded Linux system for automotive telemetry, diagnostics, and IoT integration, built with Yocto Project (Scarthgap) and targeting QEMU ARM64 and Raspberry Pi 5.

## Features
- **CAN Bus Communication**: SocketCAN-based, supports both virtual (vcan) and real CAN hardware
- **OBD-II Gateway**: Responds to standard SAE J1979 diagnostic queries
- **Data Logging**: Logs CAN frames to timestamped files
- **Telemetry Streaming**: Publishes real-time vehicle data as JSON over MQTT
- **Console Dashboard**: Terminal-based live dashboard for vehicle metrics
- **Systemd Integration**: All components run as managed services
- **Multi-platform**: Runs on QEMU (emulator) and Raspberry Pi 5 (real hardware)

## Architecture
```
┌────────────┐   CAN   ┌────────────┐   MQTT   ┌────────────┐
│ vtu-ecu-sim│◄──────►│ vtu-obdgw  │◄──────►│ vtu-telemetry│
│ (simulator)│        │ (OBD-II GW)│        │ (publisher) │
└────────────┘        └────────────┘        └────────────┘
      │                    │                    │
      ▼                    ▼                    ▼
┌────────────┐        ┌────────────┐        ┌────────────┐
│ vtu-logger │        │ vtu-console│        │ Mosquitto  │
│ (logger)   │        │ (dashboard)│        │ (broker)   │
└────────────┘        └────────────┘        └────────────┘
```

## Getting Started

### 1. Clone the repository
```bash
git clone https://github.com/almirmujanovic/vtu-project.git
cd vtu-project
```

### 2. Build for QEMU (emulator)
```bash
kas build kas-vtu-qemu.yml
```

### 3. Build for Raspberry Pi 5
```bash
kas build kas-vtu-rpi5.yml
```

### 4. Flash to SD Card (RPi5)
- Use Raspberry Pi Imager or balenaEtcher to write the `.wic.bz2` image from `build/tmp/deploy/images/raspberrypi5/` to your SD card.

### 5. Run in QEMU
```bash
kas shell kas-vtu-qemu.yml -c "runqemu qemuarm64 nographic slirp"
```

### 6. SSH into RPi5
- Connect RPi5 to Ethernet, power on, and SSH:
```bash
ssh root@<rpi-ip-address>
```

## VTU Components
- **libvtu-common**: Shared CAN/OBD-II definitions
- **vtu-ecu-sim**: CAN ECU simulator
- **vtu-obdgw**: OBD-II gateway
- **vtu-logger**: CAN data logger
- **vtu-telemetry**: MQTT publisher
- **vtu-console**: Terminal dashboard

## Technologies Used
- Yocto Project (Scarthgap)
- OpenEmbedded, BitBake, KAS
- C, CMake
- SocketCAN, OBD-II (SAE J1979)
- MQTT (paho-mqtt-c, Mosquitto)
- systemd
- QEMU, Raspberry Pi 5

## Real Hardware Support
- Works with virtual CAN (`vcan0`) and real CAN hardware (MCP2515 HAT)
- Easily extendable for other platforms

## License
MIT

## Author
Almir Mujanovic

---

**This project demonstrates end-to-end embedded Linux system development for automotive and IoT applications.**
