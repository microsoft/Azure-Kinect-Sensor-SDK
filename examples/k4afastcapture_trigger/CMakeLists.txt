# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

find_package(Threads REQUIRED)

add_executable(fastcapture_trigger trigger.cpp)
target_link_libraries(fastcapture_trigger PRIVATE 
    "${CMAKE_THREAD_LIBS_INIT}"
)