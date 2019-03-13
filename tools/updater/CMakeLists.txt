add_executable(AzureKinectFirmwareTool main.c)

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

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(
        FILES
            $<TARGET_PDB_FILE:AzureKinectFirmwareTool>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        OPTIONAL
    )
endif()
