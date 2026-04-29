SUMMARY = "EARSYS drowsiness detection service"
DESCRIPTION = "EAR-based camera drowsiness detection service for Clock OS."
HOMEPAGE = "https://github.com/WhiPaper/EARSYS"
LICENSE = "CLOSED"

SRC_URI = " \
    git://github.com/WhiPaper/EARSYS.git;protocol=https;branch=main \
    https://storage.googleapis.com/mediapipe-models/face_landmarker/face_landmarker/float16/1/face_landmarker.task;downloadfilename=face_landmarker.task;name=face_landmarker.task \
    file://earsys.service \
"

SRC_URI[face_landmarker.task.sha256sum] = "64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff"

SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git"

inherit systemd python3native

SYSTEMD_SERVICE:${PN} = "earsys.service"
SYSTEMD_AUTO_ENABLE = "enable"

# Python runtime dependencies.
RDEPENDS:${PN} += " \
    python3 \
    python3-mediapipe \
    libgl-mesa \
    libv4l \
"

do_install() {
    # ----- application sources -----
    install -d ${D}/opt/earsys

    # Copy the earsys package and the main entry-point
    cp -r ${S}/earsys   ${D}/opt/earsys/
    install -m 0644 ${S}/main.py     ${D}/opt/earsys/main.py
    install -m 0644 ${S}/requirements.txt ${D}/opt/earsys/requirements.txt

    # MediaPipe model file is unpacked under ${UNPACKDIR} (sources-unpack)
    install -m 0644 ${UNPACKDIR}/face_landmarker.task ${D}/opt/earsys/face_landmarker.task

    # ----- systemd unit -----
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/earsys.service ${D}${systemd_system_unitdir}/earsys.service
}

FILES:${PN} += " \
    /opt/earsys \
    ${systemd_system_unitdir}/earsys.service \
"
