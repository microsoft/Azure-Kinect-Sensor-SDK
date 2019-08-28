# Use the official ubuntu:18.04 image as the parent image
FROM ubuntu:18.04

# Tell Ubuntu non-interactive install
ARG DEBIAN_FRONTEND=noninteractive

# Set the working directory to /app
WORKDIR /app

# Run apt-get steps
RUN apt-get update && apt-get install -y \
    pkg-config \
    ninja-build \
    doxygen \
    clang \
    gcc-multilib \
    g++-multilib \
    python3 \
    git-lfs \
    nasm \
    cmake \
    libgl1-mesa-dev \
    libsoundio-dev \
    libvulkan-dev \
    libx11-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxrandr-dev \
    libusb-1.0-0-dev \
    libssl-dev \
    libudev-dev \
    mesa-common-dev \
    uuid-dev \
    libopencv-dev
