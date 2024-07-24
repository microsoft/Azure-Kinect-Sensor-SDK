# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(DYNLIB_SRCS dynlib_windows.c)
    set(DYNLIB_DEPENDENCIES 
        pathcch.lib)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(DYNLIB_SRCS dynlib_linux.c)
else()
    message(FATAL_ERROR "Unknown system: ${CMAKE_SYSTEM_NAME}")
endif()

add_library(k4a_dynlib STATIC
            ${DYNLIB_SRCS}
            )

# Consumers should #include <k4ainternal/dynlib.h>
target_include_directories(k4a_dynlib PUBLIC ${K4A_PRIV_INCLUDE_DIR})

target_link_libraries(k4a_dynlib PUBLIC
    k4ainternal::allocator
    k4ainternal::logging
	${DYNLIB_DEPENDENCIES})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(k4a_dynlib PUBLIC dl)
endif()

# Define alias for other targets to link against
add_library(k4ainternal::dynlib ALIAS k4a_dynlib)
