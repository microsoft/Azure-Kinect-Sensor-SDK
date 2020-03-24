#!/bin/bash

# Usage:
# ./setup-ubuntu.sh [arm64 | amd64]

# Warning! This will override your sources.list file!!

arch=amd64

# Copy off old sources.list file
cp /etc/apt/sources.list /etc/apt/sources.list.old
echo "Backed up /etc/apt/sources.list to /etc/apt/sources.list.old"

# Copy over the new file
cp sources.list /etc/apt/sources.list
echo "Overwrote /etc/apt/sources.list with sources.list"

apt-get update

apt-get install wget -y

# Add Public microsoft repo keys to the image
wget -q https://packages.microsoft.com/config/ubuntu/18.04/packages-microsoft-prod.deb
dpkg -i packages-microsoft-prod.deb

if [ "$1" = "arm64" ]; then
    arch="arm64"
fi

echo "Setting up for building $arch binaries"

dpkg --add-architecture amd64
dpkg --add-architecture arm64

apt-get update

packages=(\
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    file \
    dpkg-dev \
    qemu \
    binfmt-support \
    qemu-user-static \
    pkg-config \
    ninja-build \
    doxygen \
    clang \
    python3 \
    gcc \
    g++ \
    git \
    git-lfs \
    nasm \
    cmake \
    powershell \
    libgl1-mesa-dev:$arch \
    libsoundio-dev:$arch \
    libjpeg-dev:$arch \
    libvulkan-dev:$arch \
    libx11-dev:$arch \
    libxcursor-dev:$arch \
    libxinerama-dev:$arch \
    libxrandr-dev:$arch \
    libusb-1.0-0-dev:$arch \
    libssl-dev:$arch \
    libudev-dev:$arch \
    mesa-common-dev:$arch \
    uuid-dev:$arch )

if [ "$arch" = "amd64" ]; then
    packages+=(libopencv-dev:$arch)
fi

apt-get install -y --no-install-recommends ${packages[@]}