FILESEXTRAPATHS:prepend := "${THISDIR}:"

SRC_URI += "file://main.conf"
SRC_URI += "file://S25-JHJ.open"

do_install:append() {
    install -d ${D}${sysconfdir}/iwd
    install -m 0644 ${UNPACKDIR}/main.conf ${D}${sysconfdir}/iwd/main.conf

    install -d ${D}${localstatedir}/lib/iwd
    install -m 0644 ${UNPACKDIR}/S25-JHJ.open ${D}${localstatedir}/lib/iwd/S25-JHJ.open
}

CONFFILES:${PN} += "${sysconfdir}/iwd/main.conf"
CONFFILES:${PN} += "${localstatedir}/lib/iwd/S25-JHJ.open"

FILES:${PN} += "${localstatedir}/lib/iwd/S25-JHJ.open"