find_package(LibUSB REQUIRED)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    # Setup install
    include(GNUInstallDirs)
    install(
        FILES
            $<TARGET_FILE:LibUSB::LibUSB>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
    )
endif()