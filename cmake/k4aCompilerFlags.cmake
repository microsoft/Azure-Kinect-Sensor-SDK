# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

if (NOT ("${CMAKE_C_COMPILER_ID}" STREQUAL "${CMAKE_CXX_COMPILER_ID}"))
    message(FATAL_ERROR "C compiler (${CMAKE_C_COMPILER_ID}) does not match C++ compiler (${CMAKE_CXX_COMPILER_ID})")
endif()

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
    set(CLANG_ALL_WARNINGS "-Weverything")
    list(APPEND CLANG_ALL_WARNINGS "-Wno-missing-field-initializers") # Allow c structs without all fields initialized
    list(APPEND CLANG_ALL_WARNINGS "-Wno-reserved-id-macro") # Needed for azure-c-shared-utility which defines new macros that start with "_"
    list(APPEND CLANG_ALL_WARNINGS "-Wno-gnu-zero-variadic-macro-arguments") # Needed too allow variadic macros with zero args
    list(APPEND CLANG_ALL_WARNINGS "-Wno-extra-semi") # Allow for multiple semi-colons in a row
    list(APPEND CLANG_ALL_WARNINGS "-Wno-c++98-compat-pedantic") # Allow commas on the last enum value
    list(APPEND CLANG_ALL_WARNINGS "-Wno-padded") # Do not warn about inserted padding to structs
    list(APPEND CLANG_ALL_WARNINGS "-Wno-switch-enum") # Do not warn about missing case statements in enums
    list(APPEND CLANG_ALL_WARNINGS "-Wno-old-style-cast") # Allow old style c casts
    list(APPEND CLANG_ALL_WARNINGS "-Wno-global-constructors") # Allow global constructors. Needed for gtest
    list(APPEND CLANG_ALL_WARNINGS "-Wno-newline-eof") # Allow no newline at eof. Needed for azure-c-utility
    list(APPEND CLANG_ALL_WARNINGS "-Wno-exit-time-destructors") # Allow exit time destructors. Needed for spdlog
    list(APPEND CLANG_ALL_WARNINGS "-Wno-weak-vtables") # Allow weak vtables. Needed for spdlog
    list(APPEND CLANG_ALL_WARNINGS "-Wno-undef") # Allow undefined macros. Needed for azure-c-shared-utility
    list(APPEND CLANG_ALL_WARNINGS "-Wno-disabled-macro-expansion") # Allow recursive macro expansion
    list(APPEND CLANG_ALL_WARNINGS "-Wno-documentation-unknown-command") # Allow undocumented documentation commands used by doxygen
    list(APPEND CLANG_ALL_WARNINGS "-Wno-covered-switch-default") # Allow default: in switch statements that cover all enum values
    list(APPEND CLANG_ALL_WARNINGS "-Wno-unreachable-code-break") # Allow break even if it is unreachable
    list(APPEND CLANG_ALL_WARNINGS "-Wno-double-promotion") # Allow floats to be promoted to doubles. Needed for isnan() on some systems
    if (NOT (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "5.0.0"))
        # Added in clang 5
        list(APPEND CLANG_ALL_WARNINGS "-Wno-zero-as-null-pointer-constant") # Allow zero as nullptr
    endif()
    if (NOT (${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS "8.0.0"))
        # Added in clang 8
        list(APPEND CLANG_ALL_WARNINGS "-Wno-extra-semi-stmt") # Allow semi-colons to be used after #define's
        list(APPEND CLANG_ALL_WARNINGS "-Wno-atomic-implicit-seq-cst") # Allow use of __sync_add_and_fetch() atomic
    endif()
    set(CLANG_WARNINGS_AS_ERRORS "-Werror")
    add_compile_options(${CLANG_ALL_WARNINGS})
    add_compile_options(${CLANG_WARNINGS_AS_ERRORS})
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(GNU_ALL_WARNINGS "-Wall" "-Wextra")
    list(APPEND GNU_ALL_WARNINGS "-Wno-missing-field-initializers") # Allow c structs without all fields initialized
    set(GNU_WARNINGS_AS_ERRORS "-Werror")
    add_compile_options(${GNU_ALL_WARNINGS})
    add_compile_options(${GNU_WARNINGS_AS_ERRORS})
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set(MSVC_ALL_WARNINGS "/W4" "/wd4200") #Note: allow zero length arrays
    set(MSVC_WARNINGS_AS_ERRORS "/WX")
    string(REGEX REPLACE " /W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE " /W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    add_compile_options(${MSVC_ALL_WARNINGS})
    add_compile_options(${MSVC_WARNINGS_AS_ERRORS})
else()
    message(FATAL_ERROR "Unknown C++ compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()
