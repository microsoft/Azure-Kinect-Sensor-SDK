# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set(SOURCE_FILES
    main.cpp
    gpudepthtopointcloudconverter.cpp
    k4aaudiochanneldatagraph.cpp
    k4aaudiomanager.cpp
    k4aaudiowindow.cpp
    k4acolorimageconverter.cpp
    k4adevicedockcontrol.cpp
    k4afilepicker.cpp
    k4aimguiextensions.cpp
    k4aimugraph.cpp
    k4aimugraphdatagenerator.cpp
    k4aimuwindow.cpp
    k4alogdockcontrol.cpp
    k4amicrophone.cpp
    k4amicrophonelistener.cpp
    k4apointcloudrenderer.cpp
    k4apointcloudviewcontrol.cpp
    k4apointcloudvisualizer.cpp
    k4apointcloudwindow.cpp
    k4arecordingdockcontrol.cpp
    k4asourceselectiondockcontrol.cpp
    k4atypeoperators.cpp
    k4avideowindow.cpp
    k4aviewer.cpp
    k4aviewerimage.cpp
    k4aviewerlogmanager.cpp
    k4aviewererrormanager.cpp
    k4aviewersettingsmanager.cpp
    k4awindowmanager.cpp
    k4awindowdock.cpp
    k4awindowset.cpp
    perfcounter.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
)

# Include ${CMAKE_CURRENT_BINARY_DIR}/version.rc in the target's sources
# to embed version information
set(K4A_FILEDESCRIPTION "Azure Kinect Viewer")
set(K4A_ORIGINALFILENAME "k4aviewer.exe")
set(K4A_USE_ICON 1)
configure_file(
    ${K4A_VERSION_RC}
    ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    @ONLY
    )

find_package(OpenGL REQUIRED)
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}
)

set(EXTERNAL_LIBRARIES
    k4a::k4a
    k4a::k4arecord
    imgui::imgui
    libjpeg-turbo::libjpeg-turbo
    libsoundio::libsoundio
    LibUSB::LibUSB
    libyuv::libyuv
    glfw::glfw
    ${OPENGL_LIBRARIES}
)

# On Windows, we need to call into setupapi to get USB container ID information
# so we can correlate K4A microphone arrays and K4A depth cameras/sensors
#
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    list(APPEND SOURCE_FILES
        platform/windows/filesystem17.cpp
        platform/windows/k4adevicecorrelator.cpp
        platform/windows/wmain.cpp
    )

    list(APPEND EXTERNAL_LIBRARIES
        setupapi.lib
        pathcch.lib
        Shlwapi.lib
    )
else()
    list(APPEND SOURCE_FILES
        platform/linux/filesystem17.cpp
        platform/linux/k4adevicecorrelator.cpp
    )

    # This is a bit gross, but libsoundio has a file called endian.h in its
    # include path, which conflicts with a system file with the same name on
    # Linux.  On Ubuntu 16.04, this causes a struct in waitstatus.h to have
    # all its members defined twice (once for each endianness), which results
    # in a build break.  It looks like there's not a built-in way to pass
    # idirafter in cmake like you can with target_include_directories, so
    # anyone that uses libsoundio has to do this.  Fortunately, that should
    # just be k4aviewer
    #
    add_compile_options("-idirafter/usr/include/soundio")

endif()

add_executable(k4aviewer WIN32 ${SOURCE_FILES})
target_link_libraries(k4aviewer PRIVATE ${EXTERNAL_LIBRARIES})


# Setup install
include(GNUInstallDirs)

install(
    TARGETS
        k4aviewer
    RUNTIME DESTINATION
        ${CMAKE_INSTALL_BINDIR}
    COMPONENT
        tools
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    install(
        FILES
            $<TARGET_PDB_FILE:k4aviewer>
        DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        COMPONENT
            tools
        OPTIONAL
    )
endif()
