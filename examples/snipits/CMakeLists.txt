# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(code_snipits main.c)
target_link_libraries(code_snipits PRIVATE k4a::k4a
    )