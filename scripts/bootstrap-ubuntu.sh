#!/usr/bin/env bash

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Setup machine to build amd64 and i386
sudo dpkg --add-architecture amd64

# Update list of packages
sudo apt update

# Install tools needed to build
sudo apt install -y \
    clang \
    cmake \
    doxygen \
    g++-multilib \
    gcc-multilib \
    git-lfs \
    nasm \
    ninja-build \
    pkg-config \
    python3

# Install libraries needed to build
sudo apt install -y \
    libgl1-mesa-dev \
    libsoundio-dev \
    libssl-dev \
    libudev-dev \
    libusb-1.0-0-dev \
    libvulkan-dev \
    libx11-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxrandr-dev \
    mesa-common-dev \
    uuid-dev
