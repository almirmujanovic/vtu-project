SUMMARY = "VTU Console Dashboard"
DESCRIPTION = "Terminal-based vehicle dashboard showing live CAN bus data"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# No external dependencies - pure ANSI terminal output

SRC_URI = " \
    file://CMakeLists.txt \
    file://src/console_main.c \
"

S = "${WORKDIR}"

inherit cmake