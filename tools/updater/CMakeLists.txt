# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(AzureKinectFirmwareTool main.c ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

# Include ${CMAKE_CURRENT_BINARY_DIR}/version.rc in the target's sources
# to embed version information
set(K4A_FILEDESCRIPTION "Azure Kinect Firmware Tool")
set(K4A_ORIGINALFILENAME "AzureKinectFirmwareTool.exe")
configure_file(
    ${K4A_VERSION_RC}
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
    )

target_compile_definitions(AzureKinectFirmwareTool PRIVATE _CRT_SECURE_NO_WARNINGS)

target_link_libraries(AzureKinectFirmwareTool PRIVATE
    k4ainternal::depth_mcu
    k4ainternal::color_mcu
    k4ainternal::firmware
    k4ainternal::logging
    k4ainternal::usb_cmd)

# Setup install
include(GNUInstallDirs)

install(
    TARGETS
        AzureKinectFirmwareTool
    RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    COMPONENT
        tools
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(
        FILES
            $<TARGET_PDB_FILE:AzureKinectFirmwareTool>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        COMPONENT
            tools
        OPTIONAL
    )
endif()
