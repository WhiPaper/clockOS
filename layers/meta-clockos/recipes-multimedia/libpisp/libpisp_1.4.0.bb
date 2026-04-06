SUMMARY = "libpisp - PISP library"
DESCRIPTION = "Library for PISP pipeline handler"

LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=3417a46e992fdf62e5759fba9baef7a7"

SRC_URI = "git://github.com/raspberrypi/libpisp.git;protocol=https;branch=main"
PV = "1.4.0+git${SRCPV}"
SRCREV = "cc4b18f7da24a36f9c9d2932fe97bd269104a736"

inherit meson pkgconfig

DEPENDS = "\
  boost \
  nlohmann-json \
"

CXXFLAGS:append = " -Wno-unused-result "

INSANE_SKIP:${PN} = "buildpaths"
INSANE_SKIP:${PN}-dev = "buildpaths"