SUMMARY = "VTU MQTT Telemetry Publisher"
DESCRIPTION = "Publishes vehicle telemetry data to MQTT broker for remote monitoring"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "paho-mqtt-c"

SRC_URI = " \
    file://CMakeLists.txt \
    file://src/telemetry_main.c \
    file://vtu-telemetry.service \
"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "vtu-telemetry.service"
SYSTEMD_AUTO_ENABLE = "disable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/vtu-telemetry.service ${D}${systemd_system_unitdir}/
}

RDEPENDS:${PN} = "paho-mqtt-c"