SUMMARY = "Device Tree overlay for pwm-beeper"
DESCRIPTION = "Compiles and installs the buzzer-overlay.dts to enable pwm-beeper on RPi5."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

DEPENDS += "dtc-native"

SRC_URI = " \
    file://buzzer-overlay.dts  \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

do_compile() {
    dtc -@ -I dts -O dtb -o ${S}/pwm-beeper.dtbo ${S}/buzzer-overlay.dts
}

do_install() {
    install -d ${D}/boot/overlays
    install -m 0644 ${S}/pwm-beeper.dtbo ${D}/boot/overlays/pwm-beeper.dtbo
}

FILES:${PN} += "/boot/overlays/pwm-beeper.dtbo"
