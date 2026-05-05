FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://timesyncd.conf"

do_install:append() {
    install -d ${D}${sysconfdir}/systemd
    install -m 0644 ${UNPACKDIR}/timesyncd.conf ${D}${sysconfdir}/systemd/timesyncd.conf
}
