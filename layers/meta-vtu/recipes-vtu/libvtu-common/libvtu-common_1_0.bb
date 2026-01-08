# Recipe for libvtu-common shared library
# This library contains CAN, OBD-II, and DTC definitions for the VTU project

SUMMARY = "VTU Common Library - CAN and OBD-II definitions"
DESCRIPTION = "Shared library containing CAN message definitions, OBD-II PID \
               formulas, and diagnostic trouble codes for the Vehicle Telemetry Unit."
HOMEPAGE = "https://github.com/almirmujanovic/vtu-project"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Source files - local files from the 'files' subdirectory
SRC_URI = "file://CMakeLists.txt \
           file://include/vtu/can_defs.h \
           file://include/vtu/obd2_pids.h \
           file://include/vtu/dtc_codes.h \
           file://src/vtu_common.c"

# S = Source directory (where BitBake unpacks/finds the source)
# WORKDIR is where BitBake stages everything for this recipe
S = "${WORKDIR}"

# Inherit the CMake class - this provides do_configure, do_compile, do_install
inherit cmake

# Additional CMake options if needed
# EXTRA_OECMAKE = "-DSOME_OPTION=ON"

# Runtime dependencies (packages needed when running, not building)
# Empty for now - this is a base library with no dependencies
RDEPENDS:${PN} = ""