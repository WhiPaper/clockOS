SUMMARY = "Passive Buzzer Character Device Driver for GPIO 13 / PWM1"
DESCRIPTION = "Out-of-tree Linux kernel module that exposes a passive buzzer \
on GPIO 13 (BCM, hardware PWM1) as /dev/buzzer. \
Controlled via ioctl: BUZZER_SET_FREQ, BUZZER_START, BUZZER_STOP, BUZZER_BEEP. \
Requires the buzzer DT overlay to be loaded (dtoverlay=buzzer)."
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

inherit module

DEPENDS += "dtc-native"

SRC_URI = " \
    file://buzzer_drv.c        \
    file://buzzer_drv.h        \
    file://Makefile            \
    file://COPYING             \
    file://buzzer.rules        \
    file://buzzer-overlay.dts  \
"

S = "${WORKDIR}/sources"
UNPACKDIR = "${S}"

do_install:append() {
    # Install shared header for userspace / other recipes
    install -d ${D}${includedir}
    install -m 0644 ${S}/buzzer_drv.h ${D}${includedir}/buzzer_drv.h

    # Install udev rule so /dev/buzzer is writable by 'video' group
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -m 0644 ${S}/buzzer.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/99-buzzer.rules

    # Install compiled DT overlay to /boot/overlays/
    install -d ${D}/boot/overlays
    install -m 0644 ${S}/buzzer.dtbo ${D}/boot/overlays/buzzer.dtbo
}

FILES:${PN}     += " \
    ${nonarch_base_libdir}/udev/rules.d/99-buzzer.rules \
    /boot/overlays/buzzer.dtbo \
"
FILES:${PN}-dev += "${includedir}/buzzer_drv.h ${includedir}/${BPN}/Module.symvers"

RDEPENDS:${PN} += "kernel-module-buzzer-drv"

KERNEL_MODULE_AUTOLOAD += "buzzer_drv"
