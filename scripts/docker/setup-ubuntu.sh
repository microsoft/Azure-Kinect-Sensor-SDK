#!/bin/bash

if [ "$1" = "" ] || [ "$2" = "" ]; then
  echo "Please execute with target device and image.
$0 target_device image

Usage:
$0 [arm64 | amd64] [ubuntu-version like 22.04]"
  exit 1
fi

arch=$1
UBUNTU_VERSION=$2

apt-get update

apt-get install wget -y

# Add Public microsoft repo keys to the image
wget -q https://packages.microsoft.com/config/ubuntu/${UBUNTU_VERSION}/packages-microsoft-prod.deb -P /tmp
dpkg -i /tmp/packages-microsoft-prod.deb

echo "Setting up for building $arch binaries"

dpkg --add-architecture $arch

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
