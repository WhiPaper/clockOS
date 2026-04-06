SUMMARY = "Clock GUI Application based on LVGL 9.5"
DESCRIPTION = "Main frontend application for Clock OS, displaying time, buzzer, and other GUI elements."
LICENSE = "CLOSED"

SRC_URI = "gitsm://github.com/WhiPaper/clock-gui.git;protocol=https;branch=master \
           file://clock-gui.service \
          "

SRCREV = "${AUTOREV}"
SRCREV_lvgl = "${AUTOREV}"
SRCREV_FORMAT = "src_lvgl"

DEPENDS += "libgpiod libdrm"
RDEPENDS:${PN} += "libgpiod libdrm"

inherit cmake pkgconfig systemd

SYSTEMD_SERVICE:${PN} = "clock-gui.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/sources/clock-gui.service ${D}${systemd_system_unitdir}/
}