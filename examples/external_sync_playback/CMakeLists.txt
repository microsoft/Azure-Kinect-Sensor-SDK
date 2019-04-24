# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(external_sync_playback main.c)
target_link_libraries(external_sync_playback 
    k4a::k4a
    k4a::k4arecord
    )
