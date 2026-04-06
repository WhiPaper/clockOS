SUMMARY = "A custom Yocto image for Clock OS (Production)"
DESCRIPTION = "Minimal production image for Clock OS on Raspberry Pi 5."
LICENSE = "MIT"

inherit core-image

IMAGE_FSTYPES = "wic"
BOOT_SPACE = "262144"

IMAGE_INSTALL += " \
    packagegroup-core-boot \
    clock-gui \
    kernel-modules \
    libgpiod \
    python3 \
    python3-numpy \
    python3-opencv \
    libpisp \
    libcamera \
    libcamera-apps \
    libdrm \
    connman \
    bluez5 \
    wpa-supplicant \
"
