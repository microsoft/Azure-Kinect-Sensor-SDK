# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(depth_ut depth_ut.cpp)

target_link_libraries(depth_ut PRIVATE
    k4ainternal::utcommon

    # Link k4ainternal::depth without transitive dependencies
    $<TARGET_FILE:k4ainternal::depth>
    # Link the dependencies of k4ainternal::depth that we do not mock
    k4ainternal::calibration
    k4ainternal::image
    k4ainternal::logging
    k4ainternal::queue)

# Include the PUBLIC and INTERFACE directories specified by k4ainternal::depth
target_include_directories(depth_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::depth,INTERFACE_INCLUDE_DIRECTORIES>)

k4a_add_tests(TARGET depth_ut TEST_TYPE UNIT)
