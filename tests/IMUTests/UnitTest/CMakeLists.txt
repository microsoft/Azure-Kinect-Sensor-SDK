# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(imu_ut imu_ut.cpp)

target_link_libraries(imu_ut PRIVATE
    k4ainternal::utcommon

    # Link k4ainternal::imu without transitive dependencies
    $<TARGET_FILE:k4ainternal::imu>

    # Link the dependencies of k4ainternal::imu that we do not mock
    k4ainternal::allocator
    k4ainternal::image
    k4ainternal::logging
    k4ainternal::queue
    k4ainternal::math
    k4ainternal::calibration
    k4ainternal::depth_mcu
    k4ainternal::usb_cmd
    )

# Include the PUBLIC and INTERFACE directories specified by k4ainternal::imu
target_include_directories(imu_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::imu,INTERFACE_INCLUDE_DIRECTORIES>)

k4a_add_tests(TARGET imu_ut TEST_TYPE UNIT)
