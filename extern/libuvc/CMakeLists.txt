if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    if (NOT TARGET uvc_static)
        set(CMAKE_BUILD_TARGET Static)
        add_subdirectory(src EXCLUDE_FROM_ALL)
    else()
        message(STATUS "uvc_static is already a target. Skipping adding it twice")
    endif()

    add_library(libuvc::libuvc ALIAS uvc_static)
    target_include_directories(uvc_static PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/include>")
    target_include_directories(uvc_static PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src/include>")

endif()

