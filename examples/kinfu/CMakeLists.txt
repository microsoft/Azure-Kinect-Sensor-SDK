# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(kinfu_example
    main.cpp)

target_link_libraries(kinfu_example PRIVATE
    k4a::k4a)
