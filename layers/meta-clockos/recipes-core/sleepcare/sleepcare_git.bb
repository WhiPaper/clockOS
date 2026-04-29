SUMMARY = "SleepCare GUI + SleepCare Core WebSocket Server"
DESCRIPTION = "LVGL-based SleepCare UI (sleepcare-gui) and SleepCare Core WebSocket \
server (sleepcare-core) built from the same git repository. \
The two binaries communicate via @sleepcare/risk Unix domain socket."
LICENSE = "CLOSED"

SRC_URI = "gitsm://github.com/WhiPaper/sleepcare.git;protocol=https;branch=master \
           file://sleepcare-gui.service"

SRCREV = "${AUTOREV}"
SRCREV_lvgl = "${AUTOREV}"
SRCREV_FORMAT = "src_lvgl"

S = "${WORKDIR}/git"

DEPENDS += "libdrm libgpiod libwebsockets openssl avahi qrencode cjson"
RDEPENDS:${PN} += "libdrm libgpiod"

inherit cmake pkgconfig systemd

SYSTEMD_SERVICE:${PN} = "sleepcare-gui.service sleepcare-core.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_configure:append() {
    cmake -S ${S}/sleepcare-core \
          -B ${B}/sleepcare-core \
          -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX="${prefix}" \
          ${EXTRA_OECMAKE}
}

do_compile:append() {
    cmake --build ${B}/sleepcare-core ${PARALLEL_MAKE}
}

do_install:append() {
    cmake --install ${B}/sleepcare-core --prefix ${D}${prefix}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/sleepcare-gui.service \
                    ${D}${systemd_system_unitdir}/
    install -m 0644 ${S}/sleepcare-core/files/sleepcare-core.service \
                    ${D}${systemd_system_unitdir}/
}