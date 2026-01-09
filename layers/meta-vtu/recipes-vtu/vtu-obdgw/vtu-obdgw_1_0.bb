SUMMARY = "VTU OBD-II Gateway"
DESCRIPTION = "Responds to standard OBD-II diagnostic requests over CAN bus"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "libvtu-common"

SRC_URI = " \
    file://CMakeLists.txt \
    file://src/obdgw_main.c \
    file://vtu-obdgw.service \
"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "vtu-obdgw.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/vtu-obdgw.service ${D}${systemd_system_unitdir}/
}

RDEPENDS:${PN} = "libvtu-common"