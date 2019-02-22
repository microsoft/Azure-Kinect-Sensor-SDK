# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_library(conn_ex_utility STATIC ConnEx.cpp)

target_include_directories(conn_ex_utility PUBLIC
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(conn_ex_utility PRIVATE
    k4ainternal::logging)

# Define alias for other targets to link against
add_library(k4ainternal::conn_ex_utility ALIAS conn_ex_utility)
