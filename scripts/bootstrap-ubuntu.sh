#!/usr/bin/env bash

# Setup machine to build amd64 and i386
sudo dpkg --add-architecture amd64

# Update list of packages
sudo apt update

# Install tools needed to build
sudo apt install -y \
    pkg-config \
    ninja-build \
    doxygen \
    clang \
    gcc-multilib \
    g++-multilib \
    python3 \
    git-lfs

# Install libraries needed to build
sudo apt install -y \
    libgl1-mesa-dev \
    libsoundio-dev \
    libvulkan-dev \
    libx11-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxrandr-dev \
    libusb-1.0-0-dev \
    libssl-dev \
    mesa-common-dev \
    uuid-dev
