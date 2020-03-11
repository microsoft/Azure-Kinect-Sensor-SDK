# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# the name of the target OS and arch
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)
SET(triple x86_64-linux-gnu)

# which compilers to use
SET(CMAKE_C_COMPILER "clang-6.0")
SET(CMAKE_C_COMPILER_TARGET ${triple})
SET(CMAKE_CXX_COMPILER "clang++-6.0")
SET(CMAKE_CXX_COMPILER_TARGET ${triple})

# save flags to cache
SET(CMAKE_C_FLAGS   ${CMAKE_C_FLAGS}   CACHE STRING "C Flags"   FORCE)
SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING "C++ Flags" FORCE)

# Tell pkgconfig to use x86_64
SET(ENV{PKG_CONFIG_PATH} "/usr/lib/x86_64-linux-gnu/pkgconfig")
