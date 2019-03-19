# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# FindLibUSB.cmake
#
# Cross platform module to find the libusb library
#
# This will define the following variables
#
#   LibUSB_FOUND
#
# and the following imported targets
#
#   LibUSB::LibUSB

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")

    set(LIBUSB_PREBUILT_WIN_LIB_URL https://github.com/libusb/libusb/releases/download/v1.0.22/libusb-1.0.22.7z)
    get_filename_component(LIBUSB_7ZIP_FILENAME ${LIBUSB_PREBUILT_WIN_LIB_URL} NAME)
    set(LIBUSB_7ZIP ${CMAKE_CURRENT_BINARY_DIR}/${LIBUSB_7ZIP_FILENAME})

    # For Windows builds, download pre-built libusb libraries
    include(FetchContent)
    FetchContent_Declare(
        libusb_prebuilt_win_lib
        URL ${LIBUSB_PREBUILT_WIN_LIB_URL}
        URL_HASH MD5=750E64B45ACA94FAFBDFF07171004D03
    )

    FetchContent_GetProperties(libusb_prebuilt_win_lib)
    if (NOT libusb_prebuilt_win_lib_POPULATED)
        FetchContent_Populate(
            libusb_prebuilt_win_lib
        )
    endif()

    # Check for path to include directory
    find_path(LIBUSB_INCLUDE_DIR
        NAMES
            libusb.h
        PATHS
            ${libusb_prebuilt_win_lib_SOURCE_DIR}/include
        PATH_SUFFIXES
            libusb-1.0
    )

    if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(LIBUSB_LIB_PATH "${libusb_prebuilt_win_lib_SOURCE_DIR}/MS32/dll")
    elseif ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(LIBUSB_LIB_PATH "${libusb_prebuilt_win_lib_SOURCE_DIR}/MS64/dll")
    else()
        message(FATAL_ERROR "Unsupported size of void ptr: ${CMAKE_SIZEOF_VOID_P}")
    endif()

    find_library(LIBUSB_LIB
        NAMES
            libusb-1.0
        PATHS
            ${LIBUSB_LIB_PATH}
    )

    if ("${LIBUSB_LIB}" STREQUAL "LIBUSB_LIB-NOTFOUND")
        message(FATAL_ERROR "LibUSB not found")
    endif()

else()
    # For linux builds, find installed libusb libraries
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PC_LibUSB REQUIRED "libusb-1.0")

    find_path(LIBUSB_INCLUDE_DIR
        NAMES
            libusb.h
        HINTS
            ${PC_LibUSB_INCLUDE_DIRS}
        PATH_SUFFIXES
            libusb-1.0
    )

    find_library(LIBUSB_LIB
        NAMES
            ${PC_LibUSB_LIBRARIES}
        HINTS
            ${PC_LibUSB_LIBDIR}
            ${PC_LibUSB_LIBRARY_DIRS}
    )

endif()

mark_as_advanced(LIBUSB_INCLUDE_DIR)
mark_as_advanced(LIBUSB_LIB)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUSB DEFAULT_MSG LIBUSB_INCLUDE_DIR LIBUSB_LIB)

if (LibUSB_FOUND AND NOT TARGET LibUSB::LibUSB)
    add_library(LibUSB::LibUSB SHARED IMPORTED)

    # On Windows copy the libUSB dll to the bin directory
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        # Use REGEX instead of NAME_WE since LibUsb-1.0.lib considers the extension "0.lib"
        string(REGEX REPLACE "^.*/([^/]*)\\.lib$" "\\1" LibUSB_NAME ${LIBUSB_LIB})
        get_filename_component(LibUSB_DIRECTORY ${LIBUSB_LIB} DIRECTORY)
        set(LibUSB_SHARED_PATH ${LibUSB_DIRECTORY}/${LibUSB_NAME}.dll)
    else(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        set(LibUSB_SHARED_PATH ${LIBUSB_LIB})
    endif()


    set_target_properties(LibUSB::LibUSB PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LIBUSB_INCLUDE_DIR}"
        IMPORTED_IMPLIB "${LIBUSB_LIB}"
        IMPORTED_LOCATION "${LibUSB_SHARED_PATH}")

    if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        include(CopyImportedBinary)
        copy_imported_binary(TARGET LibUSB::LibUSB)
    endif()
endif()