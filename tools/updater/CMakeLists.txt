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

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

target_link_libraries(AzureKinectFirmwareTool PRIVATE
    k4ainternal::depth_mcu
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
)

install(
    FILES
        firmware/Firmware_Composite_MSFTCert_Debug_TestSigned_1.105.785813.5006.27_6109.07_0121.bin
        firmware/Firmware_Composite_MSFTCert_Release_MSFTSigned_1.5.786013_5006.27_6109.07_0122.bin
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/k4a/firmware
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(
        FILES
            $<TARGET_PDB_FILE:AzureKinectFirmwareTool>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        OPTIONAL
    )
endif()
