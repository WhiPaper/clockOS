SUMMARY = "MediaPipe is a cross-platform framework for building multimodal applied machine learning pipelines."
HOMEPAGE = "https://github.com/google-ai-edge/mediapipe"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=d273d63619c9ae31f2d050730753c063"

PYPI_PACKAGE = "mediapipe"
SRC_URI = "https://files.pythonhosted.org/packages/a8/f2/c8f62565abc93b9ac6a9936856d3c3c144c7f7896ef3d02bfbfad2ab6ee7/${PYPI_PACKAGE}-${PV}-cp312-cp312-manylinux_2_17_aarch64.manylinux2014_aarch64.whl;downloadfilename=${PYPI_PACKAGE}-${PV}.whl"
SRC_URI[sha256sum] = "09cbf7dc1f9a2deeaaac687e5f982836623def4cbd3e827d95f86f42450d2dd1"

inherit python3native

DEPENDS += "python3-pip-native"

RDEPENDS:${PN} += " \
    python3-core \
    python3-numpy \
    python3-protobuf \
    python3-opencv \
    python3-pillow \
"

do_unpack[noexec] = "1"

do_install() {
    install -d ${D}${PYTHON_SITEPACKAGES_DIR}
    ${STAGING_BINDIR_NATIVE}/python3-native/python3 -m pip install \
        ${WORKDIR}/mediapipe-0.10.18.whl \
        --target=${D}${PYTHON_SITEPACKAGES_DIR} \
        --no-deps \
        --no-cache-dir
}

INSANE_SKIP:${PN} += "already-stripped dev-so"
