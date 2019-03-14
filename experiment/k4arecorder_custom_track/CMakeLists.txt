# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(k4arecorder_custom_track main.cpp)

target_link_libraries(k4arecorder_custom_track PRIVATE
    k4a::k4a
    k4a::k4arecord
    )
