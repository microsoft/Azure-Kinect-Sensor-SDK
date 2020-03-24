# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Minimum Version determined by the following dev environments
# Ubuntu 16.04 (Xenial) - CMake 3.5.1
# Ubuntu 18.04 (Bionic) - CMake 3.10.2
# Visual Studio 2017 15.3 - CMake 3.8
# Visual Studio 2017 15.4 - CMake 3.9
# Visual Studio 2017 15.7 - CMake 3.11
cmake_minimum_required(VERSION 3.9.0)

cmake_policy(SET CMP0048 NEW)

# Add this repository's cmake modules to CMAKE_MODULE_PATH
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)

# If no project name is set, we are the root project
if (NOT CMAKE_PROJECT_NAME)
    # Set the default build type (if not already set)
    include(DefaultBuildType)
endif()

project(K4A LANGUAGES C CXX
    VERSION 1.4)

option(K4A_BUILD_DOCS "Build K4A doxygen documentation" OFF)
option(K4A_MTE_VERSION "Skip FW version check" OFF)
option(K4A_SOURCE_LINK "Enable source linking on MSVC" OFF)

include(GitCommands)

# Set the project version
include(K4AProjectVersion)

# Default to not embed an icon in resources
set(K4A_USE_ICON 0)
set(K4A_ICON_PATH ${CMAKE_CURRENT_LIST_DIR}/kinect-viewer.ico)

set(PROJ_DIR ${CMAKE_CURRENT_LIST_DIR})
set(INCLUDE_DIR ${PROJ_DIR}/include)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Before CMake 3.14 setting CMAKE_POSITION_INDEPENDENT_CODE did not set the
# "-pie" flag for GCC or Clang
if("${CMAKE_VERSION}" VERSION_LESS "3.14.0")
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    # Turn off incremental linking
    include(MSVCLinkerFlags)

    # Enable source linking
    # NOTE: Dependencies are not properly setup here.
    #       Currently, CMake does not know to re-link if SOURCE_LINK_JSON changes
    #       Currently, CMake does not re-generate SOURCE_LINK_JSON if git's HEAD changes
    if (K4A_SOURCE_LINK)
        if ("${CMAKE_C_COMPILER_VERSION}" VERSION_GREATER_EQUAL "19.20")
            include(SourceLink)
            file(TO_NATIVE_PATH "${PROJECT_BINARY_DIR}/source_link.json" SOURCE_LINK_JSON)
            source_link(${PROJECT_SOURCE_DIR} ${SOURCE_LINK_JSON})
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SOURCELINK:${SOURCE_LINK_JSON}")
            set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SOURCELINK:${SOURCE_LINK_JSON}")
        else()
            message(WARNING "Disabling SourceLink due to old version of MSVC. Please update to VS2019!")
        endif()
    endif()

    # Include hashes of source files in the PDB
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /ZH:SHA_256")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /ZH:SHA_256")
endif()

# If using clang or GCC, only linked shared libraries if needed
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed")
endif()

# If using clang or GCC, be sure to include a build id
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--build-id")
endif()

# If using clang or GCC, set default visibilty to hidden
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
endif()

# Find all dependencies
add_subdirectory(extern)

# Don't enable testing until after building dependencies
enable_testing()

# Turn on compiler flags for our code
include(k4aCompilerFlags)

# Source for the K4A SDK
set(K4A_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/include)

# Source for the common version resource file
set(K4A_VERSION_RC ${CMAKE_CURRENT_LIST_DIR}/version.rc.in)

if ("${K4A_ENABLE_LEAK_DETECTION_CMAKE}" STREQUAL "1")
    add_definitions(-DK4A_ENABLE_LEAK_DETECTION)
endif()

# Sets the RUNPATH entry in the .dynamic section of an elf. RUNPATH is
# interpreted by the linux loader as an additional path to search for shared
# objects. $ORIGIN is a special setting telling the loader to search the path
# relative to the exectuable.
#
# These specific settings tell the loader to search the directory of the
# executable for shared objects. This is done on Linux to emulate the default
# behavior of the Windows loader, which searches for DLLs in the path of the
# executable.
#
# We only set RPATH for build since our libs and executables are put in the
# same folder.
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    set(CMAKE_BUILD_RPATH "\$ORIGIN")
endif()

include(DetermineTargetArch)
determine_target_arch(TARGET_ARCH)

# CMake doesn't set the target processor correctly for MSVC
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    if ("${CMAKE_CONFIGURATION_TYPES}" STREQUAL "")
        set(K4A_BINARY_DIR_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        set(K4A_BINARY_DIR_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    else()
        set(K4A_BINARY_DIR_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug)
        set(K4A_BINARY_DIR_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/RelWithDebInfo)
    endif()

    # Check what architecture we are building for. This assumes all 64-bit architectures are amd64, which will break
    # if we decide to support arm.
    if ("${TARGET_ARCH}" STREQUAL "x86_64")
        configure_file(k4a.props.in ${CMAKE_CURRENT_SOURCE_DIR}/src/csharp/k4a.x64.props)
        configure_file(StubGenerator.xml.in ${CMAKE_CURRENT_SOURCE_DIR}/src/csharp/StubGenerator.x64.xml)
    elseif("${TARGET_ARCH}" STREQUAL "i686")
        configure_file(k4a.props.in ${CMAKE_CURRENT_SOURCE_DIR}/src/csharp/k4a.x86.props)
        configure_file(StubGenerator.xml.in ${CMAKE_CURRENT_SOURCE_DIR}/src/csharp/StubGenerator.x86.xml)
    else()
        message(FATAL_ERROR "Unknown architecture for MSVC: ${TARGET_ARCH}")
    endif()
endif()

add_subdirectory(examples)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)

if (K4A_BUILD_DOCS)
    find_package(Doxygen 1.8.14 EXACT)
    if (DOXYGEN_FOUND)
        set(DOXYGEN_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/mainpage.md) 
        set(DOXYGEN_SOURCES 
            ${CMAKE_CURRENT_SOURCE_DIR}/include/k4a 
            ${CMAKE_CURRENT_SOURCE_DIR}/include/k4arecord 
            ${CMAKE_CURRENT_SOURCE_DIR}/src/csharp/sdk
            ${DOXYGEN_MAINPAGE})
        set(DOXYGEN_LAYOUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/doxygen/DoxygenLayout.xml)
        # These variables are used in Doxyfile.in
        string(REPLACE ";" " " DOXYGEN_INPUT "${DOXYGEN_SOURCES}")
        set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/docs)
        set(DOXYGEN_PROJECT_LOGO ${CMAKE_CURRENT_SOURCE_DIR}/docs/logo.png)
        set(DOXYGEN_TEMPLATE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/doxygen)
        set(DOXYGEN_PROJECT_NUMBER ${SOURCE_BRANCH})

        configure_file(doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        add_custom_command(
            OUTPUT ${DOXYGEN_OUTPUT_DIRECTORY}/html/index.html
            COMMAND ${DOXYGEN_EXECUTABLE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            MAIN_DEPENDENCY doxygen/Doxyfile.in
            DEPENDS ${DOXYGEN_SOURCES}
            )
    add_custom_target(k4adocs ALL DEPENDS ${DOXYGEN_OUTPUT_DIRECTORY}/html/index.html)

    endif()
endif()

option(K4A_VALIDATE_CLANG_FORMAT "Validate clang-format results as part of build" Yes)

set (CLANG_FORMAT_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/examples
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tools)

find_program(CLANG_FORMAT clang-format DOC "Clang format tool")
if (${CLANG_FORMAT} STREQUAL "CLANG_FORMAT-NOTFOUND")
    message(STATUS "Clang-format not found")
else()

    set(VALID_CLANG_FORMAT_VERSION "6\.0\.0")
    execute_process(COMMAND ${CLANG_FORMAT} -version OUTPUT_VARIABLE CLANG_VERSION_STRING)
    message(STATUS "clang-format version: ${CLANG_VERSION_STRING}")

    if (${CLANG_VERSION_STRING} MATCHES "^.*${VALID_CLANG_FORMAT_VERSION}.*$")
        find_package(Python3 COMPONENTS Interpreter)
        if (NOT ${Python3_FOUND})
            message(FATAL_ERROR "Could not find Python3")
        endif()


        set(CLANG_FORMAT_SOURCES)
        foreach(root ${CLANG_FORMAT_ROOT})
            file(GLOB_RECURSE CLANG_FORMAT_SOURCES_X CONFIGURE_DEPENDS
                ${root}/*.c
                ${root}/*.cpp
                ${root}/*.h
                ${root}/*.hpp)
            list(APPEND CLANG_FORMAT_SOURCES ${CLANG_FORMAT_SOURCES_X})
        endforeach()

        set(VALIDATE_RESULT_LIST)
        set(REFORMAT_RESULT_LIST)
        foreach(file ${CLANG_FORMAT_SOURCES})
            file(RELATIVE_PATH filerelpath ${CMAKE_CURRENT_SOURCE_DIR} ${file})

            set(validateoutput "${CMAKE_CURRENT_BINARY_DIR}/${filerelpath}.clang-validate-result")
            set(reformatoutput "${CMAKE_CURRENT_BINARY_DIR}/${filerelpath}.clang-reformat-result")

            add_custom_command(
                OUTPUT ${validateoutput}
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateFormat.py --clangformat "${CLANG_FORMAT}" --file ${file} --output ${validateoutput}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DEPENDS ${file}
                COMMENT "Validating format of ${file}"
                )
            list(APPEND VALIDATE_RESULT_LIST ${validateoutput})


            add_custom_command(
                OUTPUT ${reformatoutput}
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateFormat.py --reformat --clangformat "${CLANG_FORMAT}" --file ${file} --output ${reformatoutput}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
                DEPENDS ${file}
                COMMENT "Reformatting ${file}"
                )
            list(APPEND REFORMAT_RESULT_LIST ${reformatoutput})

        endforeach()

        if (${K4A_VALIDATE_CLANG_FORMAT})
            add_custom_target(validateclangformat ALL DEPENDS ${VALIDATE_RESULT_LIST})
        else()
            add_custom_target(validateclangformat DEPENDS ${VALIDATE_RESULT_LIST})
        endif()

        add_custom_target(clangformat DEPENDS ${REFORMAT_RESULT_LIST})
    else()
        message(STATUS "Need clang-format version ${VALID_CLANG_FORMAT_VERSION}")
        message(STATUS "Not validating source format")
    endif()

endif()

# Generate .NET Version file.
configure_file(VersionInfo.cs.in ${CMAKE_CURRENT_SOURCE_DIR}/src/csharp/VersionInfo.cs)

# Packaging support
set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ             GROUP_EXECUTE
    WORLD_READ             WORLD_EXECUTE)
set(CPACK_PACKAGE_VENDOR "Microsoft")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C/C++ SDK for Kinect for Azure")
set(CPACK_PACKAGE_DESCRIPTION "C/C++ SDK for Kinect for Azure")
set(CPACK_PACKAGE_VERSION_MAJOR ${K4A_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${K4A_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${K4A_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${K4A_VERSION_STR})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_NAME "k4asdk")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${K4A_VERSION_STR}.${CMAKE_SYSTEM_NAME}.${TARGET_ARCH}.${CMAKE_BUILD_TYPE}")

set(CPACK_COMPONENTS_GROUPING "ONE_PER_GROUP")

set(CPACK_GENERATOR "TGZ" "ZIP")

include(CPack)

cpack_add_component(
    runtime
    DISPLAY_NAME
        Runtime
    DESCRIPTION
        "Dynamic Libraries for Azure Kinect Runtime"
    REQUIRED)

cpack_add_component(
    development
    DISPLAY_NAME
        Development
    DESCRIPTION
        "Headers and cmake files needed for Azure Kinect Development"
    REQUIRED
    DEPENDS
        runtime)

cpack_add_component(
    tools
    DISPLAY_NAME
        Tools
    DESCRIPTION
        "Tools for Azure Kinect Development"
    REQUIRED
    DEPENDS
        runtime)
