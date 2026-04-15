SUMMARY = "A custom Yocto image for Clock OS (Production)"
DESCRIPTION = "Minimal production image for Clock OS on Raspberry Pi 5."
LICENSE = "MIT"

inherit core-image

IMAGE_FSTYPES = "wic"
BOOT_SPACE = "262144"

IMAGE_INSTALL += " \
    packagegroup-core-boot \
    clock-gui \
    earsys \
    kernel-modules \
    python3 \
    libpisp \
    libcamera \
    libcamera-apps \
    bluez5 \
    iwd \
    linux-firmware-rpidistro-bcm43455 \
"
