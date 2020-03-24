# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Create K4ARecord library
add_library(k4arecord SHARED
            playback.cpp
            record.cpp
            dll_main.c
            ${CMAKE_CURRENT_BINARY_DIR}/version.rc
            )

# Generate k4arecord_export.h
# This is a platform specific header defining the macros for the export functions
# of the shared library. This header is referenced by k4arecord headers and needs to be installed
# with the SDK
include(GenerateExportHeader)
generate_export_header(k4arecord
    EXPORT_FILE_NAME "include/k4arecord/k4arecord_export.h")

# Include ${CMAKE_CURRENT_BINARY_DIR}/version.rc in the target's sources
# to embed version information
set(K4A_FILEDESCRIPTION "Azure Kinect Recording SDK")
set(K4A_ORIGINALFILENAME "k4arecord.dll")
configure_file(
    ${K4A_VERSION_RC}
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
    )

# ${K4ARECORD_INCLUDE_DIR} contains k4arecord/record.h and k4arecord/playback.h, which are the primary headers consumers should include
# ${CMAKE_CURRENT_BINARY_DIR}/include contains the generated k4arecord/k4arecord_export.h
target_include_directories(k4arecord PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<BUILD_INTERFACE:${K4ARECORD_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
    )

# Link in libraries
target_link_libraries(k4arecord PRIVATE
    k4ainternal::record
    k4ainternal::playback
    k4ainternal::logging
    ebml::ebml
    matroska::matroska
)

target_link_libraries(k4arecord PUBLIC
    k4a::k4a
)

# Define alias for k4arecord
add_library(k4a::k4arecord ALIAS k4arecord)

set_target_properties(
    k4arecord
    PROPERTIES
        VERSION
            "${K4A_VERSION_MAJOR}.${K4A_VERSION_MINOR}.${K4A_VERSION_PATCH}"
        SOVERSION
            "${K4A_VERSION_MAJOR}.${K4A_VERSION_MINOR}")

set(NAMELINK_IF_AVAILABLE)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    set(NAMELINK_IF_AVAILABLE NAMELINK_COMPONENT development)
endif()

# Setup install
include(GNUInstallDirs)
install(
    TARGETS
        k4arecord
    EXPORT
        k4arecordTargets
    LIBRARY
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}
        COMPONENT
            runtime
        ${NAMELINK_IF_AVAILABLE}
    ARCHIVE
        DESTINATION
            ${CMAKE_INSTALL_LIBDIR}
        COMPONENT
            development
    RUNTIME
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        COMPONENT
            runtime
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    k4arecordConfigVersion.cmake
    VERSION ${K4A_VERSION}
    COMPATIBILITY AnyNewerVersion
    )

install(
    EXPORT
        k4arecordTargets
    FILE
        k4arecordTargets.cmake
    NAMESPACE
        k4a::
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/k4arecord
    COMPONENT
        development
)

configure_file("${PROJECT_SOURCE_DIR}/cmake/k4arecordConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/k4arecordConfig.cmake" COPYONLY)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/k4arecordConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/k4arecordConfigVersion.cmake"
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/k4arecord
    COMPONENT
        development
)

install(
    FILES
        ${K4A_INCLUDE_DIR}/k4arecord/record.h
        ${K4A_INCLUDE_DIR}/k4arecord/record.hpp
        ${K4A_INCLUDE_DIR}/k4arecord/playback.h
        ${K4A_INCLUDE_DIR}/k4arecord/playback.hpp
        ${K4A_INCLUDE_DIR}/k4arecord/types.h
        ${CMAKE_CURRENT_BINARY_DIR}/include/k4arecord/k4arecord_export.h
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/k4arecord
    COMPONENT
        development
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(
        FILES
            $<TARGET_PDB_FILE:k4arecord>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        COMPONENT
            runtime
        OPTIONAL
    )
endif()
