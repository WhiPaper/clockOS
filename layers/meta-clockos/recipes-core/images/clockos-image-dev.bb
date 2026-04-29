SUMMARY = "A custom Yocto image for Clock OS (Development)"
DESCRIPTION = "Development image inheriting the production clockos-image. \
Adds SSH access, debuggers, hardware inspection tools, and serial console \
for iterative development on Raspberry Pi 5. \
Includes C/C++ toolchain (gcc, g++, cmake, gdb) for on-device development and testing."
LICENSE = "MIT"

require clockos-image.bb

# Development and debugging tools
IMAGE_INSTALL += " \
    htop \
    curl \
    libgpiod-tools \
    evtest \
    v4l-utils \
    fbida \
    i2c-tools \
    minicom \
    libdrm-tests \
    \
    cmake \
    ninja \
    ltrace \
"

IMAGE_FEATURES += " \
    ssh-server-dropbear \
    tools-debug \
    tools-sdk \
    allow-empty-password \
    empty-root-password \
    post-install-logging \
    allow-root-login \
    package-management \
    dev-pkgs \
"