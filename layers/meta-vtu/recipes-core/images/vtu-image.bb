# VTU (Vehicle Telemetry Unit) Custom Image
# 
# This image includes all VTU components for automotive telemetry
# Target: qemuarm64 (development), raspberrypi5 (production)

SUMMARY = "Vehicle Telemetry Unit - Complete System Image"
DESCRIPTION = "Embedded Linux image for automotive telemetry with CAN bus \
               support, OBD-II diagnostics, and data logging capabilities."
LICENSE = "MIT"

# Inherit from core-image - this provides the base image functionality
# core-image.bbclass sets up the basic rootfs structure
inherit core-image


# ============================================================================
# Image Features
# ============================================================================
# These are predefined feature sets that add common functionality

IMAGE_FEATURES += " \
    ssh-server-openssh \
    debug-tweaks \
    tools-debug \
"

# Feature explanations:
# - ssh-server-openssh: Include OpenSSH server for remote access
# - debug-tweaks: Allow root login without password (DEV ONLY!)
# - tools-debug: Include gdb, strace, etc.

# ============================================================================
# VTU Packages
# ============================================================================
# Our custom packages from meta-vtu layer

VTU_PACKAGES = " \
    libvtu-common \
    vtu-ecu-sim \
    vtu-obdgw \
    vtu-logger \
"

# ============================================================================
# CAN Bus Support
# ============================================================================
# Tools and kernel modules for CAN bus operation

CAN_PACKAGES = " \
    can-utils \
    iproute2 \
"

# can-utils: candump, cansend, cangen, etc.
# iproute2: ip command for network/CAN interface management

# ============================================================================
# System Utilities
# ============================================================================
# Helpful tools for development and debugging

SYSTEM_PACKAGES = " \
    htop \
    nano \
    less \
    tree \
    procps \
    util-linux \
"

# ============================================================================
# Networking (for MQTT in later phases)
# ============================================================================

NETWORK_PACKAGES = " \
    iputils-ping \
    net-tools \
"

# ============================================================================
# Combine all packages
# ============================================================================

IMAGE_INSTALL += " \
    ${VTU_PACKAGES} \
    ${CAN_PACKAGES} \
    ${SYSTEM_PACKAGES} \
    ${NETWORK_PACKAGES} \
    kernel-modules \
"

# ============================================================================
# Image Configuration
# ============================================================================

# Set root filesystem size (auto-expand, minimum 512MB)
IMAGE_ROOTFS_SIZE ?= "524288"

# Extra space for logs and data
IMAGE_ROOTFS_EXTRA_SPACE = "131072"

# ============================================================================
# Post-process commands
# ============================================================================
# Commands run after rootfs is assembled but before image is created

setup_vcan() {
    # Create a script to set up virtual CAN on boot (for QEMU)
    cat > ${IMAGE_ROOTFS}/etc/profile.d/vcan-setup.sh << 'EOF'
# Virtual CAN setup helper
# On real hardware, this would be replaced with physical CAN setup
alias vcan-up='sudo ip link add dev vcan0 type vcan && sudo ip link set up vcan0'
alias vcan-status='ip -details link show vcan0'
alias can-watch='candump vcan0'
EOF
    chmod 0644 ${IMAGE_ROOTFS}/etc/profile.d/vcan-setup.sh
}

ROOTFS_POSTPROCESS_COMMAND += "setup_vcan;"