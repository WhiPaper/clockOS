SUMMARY = "EARSYS drowsiness detection service"
DESCRIPTION = "Camera-based drowsiness detection service for Clock OS."
LICENSE = "CLOSED"

SRC_URI = "git://github.com/WhiPaper/EARSYS.git;protocol=https;branch=main \
           file://earsys.service \
          "

SRCREV = "${AUTOREV}"

RDEPENDS:${PN} += " \
    python3-core \
    python3-numpy \
    python3-opencv \
    gstreamer1.0 \
    gstreamer1.0-plugins-base-meta \
    libcamera-gst \
"

inherit systemd

SYSTEMD_SERVICE:${PN} = "earsys.service"
SYSTEMD_AUTO_ENABLE = "enable"

# Install the full app directory so model files and assets from the repo are available.
do_install() {
    install -d ${D}/opt/earsys
    cp -r --no-preserve=ownership,mode ${S}/* ${D}/opt/earsys/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/sources/earsys.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += " \
    /opt/earsys \
    ${systemd_system_unitdir}/earsys.service \
"
