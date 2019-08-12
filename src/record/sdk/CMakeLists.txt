# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Define public library
add_library(k4arecord_private SHARED
            playback.cpp
            record.cpp
            dll_main.c
            ${CMAKE_CURRENT_BINARY_DIR}/version.rc
            )

# Generate k4arecord_export_private.h
# This is a platform specific header defining the macros for the export functions
# of the shared library. This header is referenced by k4arecord headers and needs to be installed
# with the sdk
include(GenerateExportHeader)
generate_export_header(k4arecord_private
    EXPORT_FILE_NAME "include/k4arecord/k4arecord_export_private.h")

# Include ${CMAKE_CURRENT_BINARY_DIR}/version.rc in the target's sources
# to embed version information
set(K4A_FILEDESCRIPTION "Azure Kinect Recording SDK")
set(K4A_ORIGINALFILENAME "k4arecord_private.dll")
configure_file(
    ${K4A_VERSION_RC}
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
    )

# ${K4ARECORD_INCLUDE_DIR} contains k4arecord/record.h and k4arecord/playback_private.h, which are the primary headers consumers should include
# ${CMAKE_CURRENT_BINARY_DIR}/include contains the generated k4arecord/k4arecord_export_private.h
target_include_directories(k4arecord_private PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${K4ARECORD_INCLUDE_DIR}
    )

target_link_libraries(k4arecord_private PRIVATE
    k4ainternal::record
    k4ainternal::playback
    k4a::k4a
    k4ainternal::logging
    ebml::ebml
    matroska::matroska
)

# Define alias for k4arecord
add_library(k4a::k4arecord ALIAS k4arecord_private)

set_target_properties(
    k4arecord_private
    PROPERTIES
        VERSION ${K4A_VERSION}
        SOVERSION ${K4A_VERSION})

# Setup install
include(GNUInstallDirs)
install(
    TARGETS
        k4arecord_private
    LIBRARY DESTINATION
        ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION
        ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
)

install(
    FILES
        ${K4A_INCLUDE_DIR}/k4arecord/record_private.h
        ${K4A_INCLUDE_DIR}/k4arecord/playback_private.h
        ${K4A_INCLUDE_DIR}/k4arecord/types_private.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/k4arecord/k4arecord_export_private.h
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/k4arecord
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(
        FILES
            $<TARGET_PDB_FILE:k4arecord_private>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        OPTIONAL
    )
endif()
