# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(depthmcu_ut depthmcu_ut.cpp)

target_link_libraries(depthmcu_ut PRIVATE
    k4ainternal::utcommon

    # Link k4ainternal::depth without transitive dependencies
    $<TARGET_FILE:k4ainternal::depth_mcu>
    # Link the dependencies of k4ainternal::depth that we do not mock
    k4ainternal::allocator
    k4ainternal::image
    k4ainternal::logging
    )

# Include the PUBLIC and INTERFACE directories specified by k4ainternal::depth_mcu
target_include_directories(depthmcu_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::depth_mcu,INTERFACE_INCLUDE_DIRECTORIES>)

k4a_add_tests(TARGET depthmcu_ut TEST_TYPE UNIT)
