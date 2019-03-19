# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#
# Mock shared library
#
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(COLOR_MOCK_SOURCE color_mock_mediafoundation.cpp)
    set(COLOR_SYSTEM_MOCK_SOURCE color_mock_windows.cpp color_mock_windows.def)

    add_library(color_ut_mock SHARED
        ${COLOR_SYSTEM_MOCK_SOURCE})

    target_link_libraries(color_ut_mock PRIVATE
        k4ainternal::utcommon)

elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(COLOR_MOCK_SOURCE color_mock_libuvc.cpp)
endif()

#
# Color unit test
#
add_executable(color_ut color_ut.cpp ${COLOR_MOCK_SOURCE})

target_link_libraries(color_ut PRIVATE
    k4ainternal::utcommon

    # Link k4ainternal::color without transitive dependencies
    $<TARGET_FILE:k4ainternal::color>
    k4ainternal::image
    k4ainternal::queue)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(color_ut PRIVATE
        color_ut_mock
        mf.lib
        mfplat.lib
        mfreadwrite.lib
        mfuuid.lib
        cfgmgr32.lib)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(color_ut PRIVATE
        libuvc::libuvc
        libjpeg-turbo::libjpeg-turbo)
endif()

k4a_add_tests(TARGET color_ut TEST_TYPE UNIT)
