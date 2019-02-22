# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Copies the imported binary DLL to the RUNTIME output location
#
# This enables executables to be run from the output location without including the dependent DLLs in the PATH
#
# Example:

# add_library(External::Foo SHARED IMPORTED GLOBAL)
#
# set_target_properties(External::Foo PROPERTIES
#   INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
#   IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/Foo.lib"
#   IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/Foo.dll"
#   )
# include(CopyImportedBinary)
# copy_imported_binary(TARGET External::Foo)
#
#

function(copy_imported_binary)

    # Parse arguments to
    # COPY_IMPORTED_BINARY_TARGET : The imported target to copy the dll from
    cmake_parse_arguments(COPY_IMPORTED_BINARY "" "TARGET" "" ${ARGN})

    message(STATUS "Copying DLLs for ${COPY_IMPORTED_BINARY_TARGET}")

    get_property(Target_Dll_Path TARGET "${COPY_IMPORTED_BINARY_TARGET}" PROPERTY IMPORTED_LOCATION)

    # Copy the DLL to all output directories for available configs (for multi-configuration generators)
    if ("${CMAKE_CONFIGURATION_TYPES}" STREQUAL "")
        set(DLL_COPY_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

        file(MAKE_DIRECTORY "${DLL_COPY_DIRECTORY}")
        file(COPY "${Target_Dll_Path}" DESTINATION "${DLL_COPY_DIRECTORY}")

        message(STATUS "Copied dll from ${Target_Dll_Path} to ${DLL_COPY_DIRECTORY}")
    else()
        foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
            set(DLL_COPY_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG}")

            file(MAKE_DIRECTORY "${DLL_COPY_DIRECTORY}")
            file(COPY "${Target_Dll_Path}" DESTINATION "${DLL_COPY_DIRECTORY}")
            message(STATUS "Copied dll from ${Target_Dll_Path} to ${DLL_COPY_DIRECTORY}")
        endforeach(CONFIG)
    endif()
endfunction()