# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Currently the conn_ex_utility is Windows only, so these tests are only valid during a Windows build.
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")

add_executable(firmware_fw firmware_helper.cpp firmware_fw.cpp firmware_interrupt_fw.cpp)

target_compile_definitions(firmware_fw PRIVATE _CRT_SECURE_NO_WARNINGS)

target_link_libraries(firmware_fw PRIVATE
    k4ainternal::utcommon

    k4ainternal::conn_ex_utility
    k4ainternal::depth_mcu
    k4ainternal::color_mcu
    k4ainternal::calibration
    k4ainternal::firmware
    k4ainternal::logging
    k4ainternal::usb_cmd)

# Include the PUBLIC directories specified by k4ainternal::conn_ex_utility
target_include_directories(firmware_fw PRIVATE $<TARGET_PROPERTY:k4ainternal::conn_ex_utility,INTERFACE_INCLUDE_DIRECTORIES>)

k4a_add_tests(TARGET firmware_fw TEST_TYPE FIRMWARE)

endif()
