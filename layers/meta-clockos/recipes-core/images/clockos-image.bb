SUMMARY = "A custom Yocto image for Clock OS (Production)"
DESCRIPTION = "Minimal production image for Clock OS on Raspberry Pi 5."
LICENSE = "MIT"

inherit core-image

IMAGE_FSTYPES = "wic"
BOOT_SPACE = "262144"

IMAGE_INSTALL += " \
    packagegroup-core-boot \
    sleepcare \
    earsys \
    kernel-modules \
    libpisp \
    libcamera \
    libcamera-gst \
    libcamera-apps \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    bluez5 \
    iwd \
    linux-firmware-rpidistro-bcm43455 \
    mesa \
    libv4l \
    tzdata \
    tzdata-asia \
    buzzer-driver \
    buzzer-driver-dev \
    avahi-daemon \
    openssl \
    chrony \
"
