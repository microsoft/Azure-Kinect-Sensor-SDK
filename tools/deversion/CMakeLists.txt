# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(deversion main.c)

target_link_libraries(deversion PRIVATE
    k4ainternal::dynlib)
