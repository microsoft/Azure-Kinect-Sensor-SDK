# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(logging_ut logging_ut.cpp)

target_link_libraries(logging_ut PRIVATE
    k4ainternal::utcommon

    # Link k4ainternal::logging without transitive dependencies
    $<TARGET_FILE:k4ainternal::logging>
    # Link the dependencies of k4ainternal::logging that we do not mock
    )

# Include the PUBLIC and INTERFACE directories specified by k4ainternal::logging
target_include_directories(logging_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::logging,INTERFACE_INCLUDE_DIRECTORIES>)

k4a_add_tests(TARGET logging_ut TEST_TYPE UNIT)
