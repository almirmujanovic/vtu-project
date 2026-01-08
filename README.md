# Vehicle Telemetry Unit (VTU)

An embedded Linux system for automotive/avionics telemetry built with Yocto/OpenEmbedded.

## Features

- CAN bus data collection and simulation
- Real-time telemetry streaming (MQTT)
- Black-box style data logging
- Web dashboard for monitoring
- REST API for data access

## Building

### Prerequisites

- Ubuntu 22.04/24.04 or similar Linux distribution
- At least 50GB free disk space
- At least 8GB RAM (16GB recommended)
- KAS build tool (`pipx install kas`)

### Build for QEMU (ARM64)

```bash
kas build kas-vtu-qemu.yml
```

### Run in QEMU

```bash
kas shell kas-vtu-qemu.yml -c "runqemu qemuarm64 nographic"
```

### Build for Raspberry Pi 5

```bash
kas build kas-vtu-rpi5.yml
```

## Project Structure

```
vtu-project/
├── kas-vtu-qemu.yml      # KAS config for QEMU target
├── kas-vtu-rpi5.yml      # KAS config for RPi5 (later)
├── layers/               # Yocto layers (auto-fetched by KAS)
│   ├── poky/
│   ├── meta-openembedded/
│   └── meta-vtu/         # Our custom layer (will create)
├── build/                # Build output
├── downloads/            # Shared download cache
└── sstate-cache/         # Shared state cache
```

## License

MIT License - Educational project
