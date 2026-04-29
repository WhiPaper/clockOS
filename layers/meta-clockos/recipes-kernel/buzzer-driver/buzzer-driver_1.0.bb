SUMMARY = "Passive Buzzer Character Device Driver for GPIO 13 / PWM1"
DESCRIPTION = "Out-of-tree Linux kernel module that exposes a passive buzzer \
on GPIO 13 (BCM, hardware PWM1) as /dev/buzzer. \
Controlled via ioctl: BUZZER_SET_FREQ, BUZZER_START, BUZZER_STOP, BUZZER_BEEP."
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

inherit module

SRC_URI = " \
    file://buzzer_drv.c \
    file://buzzer_drv.h \
    file://Makefile    \
    file://COPYING     \
    file://buzzer.rules \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

do_install:append() {
    # Install kernel module (module.bbclass handles .ko automatically)

    # Install shared header for userspace / other recipes
    install -d ${D}${includedir}
    install -m 0644 ${S}/buzzer_drv.h ${D}${includedir}/buzzer_drv.h

    # Install udev rule so /dev/buzzer is writable by 'video' group
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -m 0644 ${S}/buzzer.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/99-buzzer.rules
}

FILES:${PN}     += "${nonarch_base_libdir}/udev/rules.d/99-buzzer.rules"
FILES:${PN}-dev += "${includedir}/buzzer_drv.h ${includedir}/${BPN}/Module.symvers"

RPROVIDES:${PN} += "kernel-module-buzzer-drv"
