# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# For code, link against the static crt on windows
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    # Remove all previously set dynamic linking flags
    string(REGEX REPLACE "/MD" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REGEX REPLACE "/MD" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REGEX REPLACE "/MD" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    string(REGEX REPLACE "/MDd" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

    string(REGEX REPLACE "/MD" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    string(REGEX REPLACE "/MD" "" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REGEX REPLACE "/MD" "" CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
    string(REGEX REPLACE "/MDd" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

    # Link against the static crt
    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MTd")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MTd")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MT")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
    endif()
endif()