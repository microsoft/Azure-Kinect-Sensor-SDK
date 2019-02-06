# External dependencies are pulled in via submodules. Lets make sure the user
# has updated all the submodules
if ((NOT K4A_TURNED_ON_GIT_SUBMODULES_RECURSE) OR
    (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/azure_c_shared/src/CMakeLists.txt"))
    find_package(Git REQUIRED QUIET)
    if (Git_FOUND)
        get_git_dir(${K4A_SOURCE_DIR} GIT_DIR)
        if (GIT_DIR)
            message(STATUS "Updating git modules...")
            execute_process(
                COMMAND
                    ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY
                    ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE
                    GIT_SUBMODULE_UPDATE_RESULT
                OUTPUT_VARIABLE
                    GIT_SUBMODULE_UPDATE_OUTPUT
            )

            if (NOT (${GIT_SUBMODULE_UPDATE_RESULT} EQUAL 0))
                message(FATAL_ERROR "git submodule update failed:
                                    ${GIT_SUBMODULE_UDPATE_OUTPUT}")
            endif()

            message(STATUS "Setting git config submodules.recurse to true")
            execute_process(
                COMMAND
                    ${GIT_EXECUTABLE} config submodule.recurse true
                WORKING_DIRECTORY
                    ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE
                    GIT_CONFIG_SUBMODULE_RESULT
                OUTPUT_VARIABLE
                    GIT_CONFIG_SUBMODULE_OUTPUT
            )

            if (NOT (${GIT_CONFIG_SUBMODULE_RESULT} EQUAL 0))
                message(FATAL_ERROR "git config submodule.recurse true failed:
                                    ${GIT_CONFIG_SUBMODULE_OUTPUT}")
            endif()

            set(K4A_TURNED_ON_GIT_SUBMODULES_RECURSE TRUE CACHE BOOL "Flag to signify we have turned on git submodule.recurse in this repo")
        endif()
    endif()
endif()
add_subdirectory(azure_c_shared)
add_subdirectory(cjson)
add_subdirectory(glfw)
add_subdirectory(googletest)
add_subdirectory(imgui)
add_subdirectory(libebml)
add_subdirectory(libjpeg-turbo)
add_subdirectory(libmatroska)
add_subdirectory(libsoundio)
add_subdirectory(libyuv)
add_subdirectory(spdlog)
add_subdirectory(libuvc)
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
    add_subdirectory(libusb)
endif()
