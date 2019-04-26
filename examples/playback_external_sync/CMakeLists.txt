# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(playback_external_sync main.c)
target_link_libraries(playback_external_sync 
    k4a::k4a
    k4a::k4arecord
    )
