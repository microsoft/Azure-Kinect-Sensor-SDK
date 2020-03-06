# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# the name of the target OS and arch
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm64)

# which compilers to use
SET(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Tell pkgconfig to use arm64
SET(ENV{PKG_CONFIG_PATH} "/usr/lib/aarch64-linux-gnu/pkgconfig")

# Tell CMake to use qemu to emulate
# Note: This should be automatically done by Ubuntu using binfmt_misc, but that
# seems to be broken on WSL (https://github.com/microsoft/WSL/issues/2620) so
# explicitly setting the emulator for now.
SET(CMAKE_CROSSCOMPILING_EMULATOR qemu-aarch64-static)
