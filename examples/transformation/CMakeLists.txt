# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(transformation_example 
    main.cpp
    transformation_helpers.cpp
    transformation_helpers.h)

target_link_libraries(transformation_example PRIVATE
    k4a::k4a
    k4a::k4arecord
    libjpeg-turbo::libjpeg-turbo
    )
