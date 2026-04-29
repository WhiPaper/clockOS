SUMMARY = "MediaPipe is a cross-platform framework for building multimodal applied machine learning pipelines."
HOMEPAGE = "https://github.com/google-ai-edge/mediapipe"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

PYPI_PACKAGE = "mediapipe"
WHL_FILENAME = "${PYPI_PACKAGE}-${PV}-cp312-cp312-manylinux_2_17_aarch64.manylinux2014_aarch64.whl"
SRC_URI = "https://files.pythonhosted.org/packages/a8/f2/c8f62565abc93b9ac6a9936856d3c3c144c7f7896ef3d02bfbfad2ab6ee7/${WHL_FILENAME}"
SRC_URI[sha256sum] = "09cbf7dc1f9a2deeaaac687e5f982836623def4cbd3e827d95f86f42450d2dd1"
S = "${UNPACKDIR}"
COMPATIBLE_HOST = "aarch64.*-linux"

inherit python3native

DEPENDS += "unzip-native"

RDEPENDS:${PN} += " \
    python3-core \
    python3-numpy \
    python3-protobuf \
    python3-opencv \
    python3-pillow \
    python3-matplotlib \
    python3-attrs \
"

do_unpack() {
    cp ${DL_DIR}/${WHL_FILENAME} ${UNPACKDIR}/
}

do_configure[noexec] = "1"
do_compile[noexec] = "1"

do_install() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    unzip -q ${UNPACKDIR}/${WHL_FILENAME} -d ${D}${PYTHON_SITEPACKAGES_DIR}
}

FILES:${PN} += " \
    ${PYTHON_SITEPACKAGES_DIR}/mediapipe \
    ${PYTHON_SITEPACKAGES_DIR}/mediapipe.libs \
    ${PYTHON_SITEPACKAGES_DIR}/mediapipe-*.dist-info \
"

INSANE_SKIP:${PN} += "already-stripped dev-so"
