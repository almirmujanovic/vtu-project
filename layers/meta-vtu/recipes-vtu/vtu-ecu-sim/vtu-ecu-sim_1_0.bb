SUMMARY = "VTU ECU Simulator"
DESCRIPTION = "Simulates vehicle ECUs broadcasting CAN data and responding to OBD-II requests"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Source files
SRC_URI = "file://CMakeLists.txt \
           file://src/ecu_sim.c \
           file://vtu-ecu-sim.service"

S = "${WORKDIR}"

# Build dependencies - packages needed to compile
# libvtu-common: our shared library
DEPENDS = "libvtu-common"

# Inherit CMake for building
inherit cmake

# Inherit systemd class for service integration
inherit systemd

# Tell systemd class which services we provide
SYSTEMD_SERVICE:${PN} = "vtu-ecu-sim.service"

# Auto-enable service at boot
SYSTEMD_AUTO_ENABLE = "enable"

# Runtime dependencies
# can-utils: provides candump, cansend for debugging
# We need kernel CAN support (built into qemuarm64 kernel)
RDEPENDS:${PN} = "libvtu-common can-utils"

# Install the systemd service file
do_install:append() {
    # Create systemd service directory
    install -d ${D}${systemd_system_unitdir}
    
    # Install our service file
    install -m 0644 ${WORKDIR}/vtu-ecu-sim.service ${D}${systemd_system_unitdir}/
}