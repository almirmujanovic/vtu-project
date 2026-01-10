SUMMARY = "VTU CAN Bus Data Logger"
DESCRIPTION = "Logs CAN bus traffic to files with timestamps for diagnostics and analysis"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
    file://CMakeLists.txt \
    file://src/logger_main.c \
    file://vtu-logger.service \
"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "vtu-logger.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/vtu-logger.service ${D}${systemd_system_unitdir}/
}