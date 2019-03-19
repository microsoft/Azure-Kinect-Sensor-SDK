# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

set (TARGET testutil)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(K4A_COLOR_SYSTEM_DIRECTORIES ${PROJECT_SOURCE_DIR}/src/color/)
    set(K4A_COLOR_SYSTEM_SOURCES ${K4A_COLOR_SYSTEM_DIRECTORIES}mfcamerareader.cpp)
    set(K4A_COLOR_SYSTEM_DEPENDENCIES 
        mf.lib
        mfplat.lib
        mfreadwrite.lib
        mfuuid.lib
        cfgmgr32.lib)
endif()

set (SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/Capture.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/CliEng.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Example.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/k4aCmd.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/UsbCmd.c"
    ${K4A_COLOR_SYSTEM_SOURCES}
    )
    
set (LIBS 
    k4a::k4a
    k4ainternal::allocator
    k4ainternal::image
    k4ainternal::logging
    k4ainternal::usb_cmd
    ${K4A_COLOR_SYSTEM_DEPENDENCIES}
    )

add_executable(${TARGET} ${SRCS})

target_include_directories(${TARGET} PRIVATE ${K4A_COLOR_SYSTEM_DIRECTORIES})
target_link_libraries(${TARGET} PRIVATE ${LIBS})
