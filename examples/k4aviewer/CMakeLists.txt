set(SOURCE_FILES
    main.cpp
    k4aaudiochanneldatagraph.cpp
    k4aaudiomanager.cpp
    k4aaudiowindow.cpp
    k4acalibrationtransformdata.cpp
    k4acapture.cpp
    k4acolorframevisualizer.cpp
    k4adepthframevisualizer.cpp
    k4adevice.cpp
    k4adeviceselectioncontrol.cpp
    k4adevicesettingscontrol.cpp
    k4aimguiextensions.cpp
    k4aimudatagraph.cpp
    k4aimusamplesource.cpp
    k4aimuwindow.cpp
    k4ainfraredframevisualizer.cpp
    k4amicrophone.cpp
    k4amicrophonelistener.cpp
    k4apointcloudrenderer.cpp
    k4apointcloudviewcontrol.cpp
    k4apointcloudvisualizer.cpp
    k4apointcloudwindow.cpp
    k4atypeoperators.cpp
    k4avideowindow.cpp
    k4aviewer.cpp
    k4aviewererrormanager.cpp
    k4aviewersettingsmanager.cpp
    k4awindowmanager.cpp
    opengltexture.cpp
    perfcounter.cpp
)

find_package(OpenGL REQUIRED)
find_package(LibUSB REQUIRED)
include_directories(
    ${OPENGL_INCLUDE_DIRS}
    ${CMAKE_CURRENT_LIST_DIR}
)

set(EXTERNAL_LIBRARIES
    k4a::k4a
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
        platform/windows/k4adevicecorrelator.cpp
        platform/windows/wmain.cpp
    )

    find_library(SETUPAPI_LIB setupapi)
    list(APPEND EXTERNAL_LIBRARIES
        ${SETUPAPI_LIB}
    )
else()
    list(APPEND SOURCE_FILES
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

