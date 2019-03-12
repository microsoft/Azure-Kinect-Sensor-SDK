# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Add shader compilation to a target
#
# target_add_shader(TARGET <target> SHADERS <shader>... [DEPENDS <depend>...])
#
# Adds build rules to combile <shaders> before building <target>. Each shader will
# generate a header output with the same name and extension ".hcs" which can be included
# in your program. The directory with the resultant .hcs files will be available in the
# include path of <target>
#
# Example:
# target_add_shader(TARGET myprogram SHADERS foo.hlsl bar.hlsl DEPENDS common.hlsl)
#
# Compiles foo.hlsl to foo.hcs and bar.hlsl to bar.hcs. Since foo.hlsl #includes common.hlsl
# common.hlsl is added to the DEPENDS. When common.hlsl is modified the incremental build will
# recompile foo.hcs, bar.hcs, and myprogram.
#
# If the ninja generator is used, depends are automatically determined when compiling the shader
#

include_guard(GLOBAL)

function(target_add_shaders)

    # Parse arguments to
    # TARGET_ADD_SHADERS_SHADERS : List of shaders to compiledshaders
    # TARGET_ADD_SHADERS_TARGET  : Target the shaders are depenent on
    # TARGET_ADD_SHADERS_DEPENDS : Additional dependencies for these shaders

    cmake_parse_arguments(TARGET_ADD_SHADERS "" "TARGET" "SHADERS;DEPENDS" ${ARGN})

    find_program(FXC fxc DOC "fx shader compiler")
    if ("${FXC}" STREQUAL "FXC-NOTFOUND")
        message(FATAL_ERROR "FX Shader compiler not found")
    endif()
    message(STATUS "Found FXC: ${FXC}")
	
	set(FXC_FLAGS "/nologo" "/Vi")
	
	if (("${CMAKE_BUILD_TYPE}" STREQUAL "Debug") AND ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC"))
	    set(FXC_FLAGS ${FXC_FLAGS} "/Zi" "/Od")
	endif()

    foreach (FILE ${TARGET_ADD_SHADERS_SHADERS})
        get_filename_component(FILE_WE ${FILE} NAME_WE)

        set(FX_HEADER ${CMAKE_CURRENT_BINARY_DIR}/compiledshaders/${FILE_WE}.hcs)

        set(DEPFILE ${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}.d)
        find_package(Python3 REQUIRED)

        # CaptureFxcDeps.py will capture the output and add any included .h files to the dependencies
        # through a depfile for ninja

        # Invoke the Shader compiler
        if (${CMAKE_GENERATOR} EQUAL "Ninja")
            add_custom_command(
                OUTPUT ${FX_HEADER}
                COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/cmake/CaptureFxcDeps.py --prefix ${PROJECT_BINARY_DIR} --depfile ${DEPFILE} --outputs ${FX_HEADER} --fxc ${FXC} ${FXC_FLAGS} /E"CSMain" /T cs_5_0 /Fh ${FX_HEADER} /Vn ${FILE_WE} ${FILE}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS ${FILE} ${TARGET_ADD_SHADERS_DEPENDS} ${PROJECT_SOURCE_DIR}/cmake/CaptureFxcDeps.py
                COMMENT "Building Shader ${FILE_WE}"
                DEPFILE ${DEPFILE}
                )
        else()
            # DEPFILE argument is not compatible with non-Ninja generators
            add_custom_command(
                OUTPUT ${FX_HEADER}
                COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/cmake/CaptureFxcDeps.py --prefix ${PROJECT_BINARY_DIR} --depfile ${DEPFILE} --outputs ${FX_HEADER} --fxc ${FXC} ${FXC_FLAGS} /E"CSMain" /T cs_5_0 /Fh ${FX_HEADER} /Vn ${FILE_WE} ${FILE}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS ${FILE} ${TARGET_ADD_SHADERS_DEPENDS} ${PROJECT_SOURCE_DIR}/cmake/CaptureFxcDeps.py
                COMMENT "Building Shader ${FILE_WE}"
                )
        endif()

        # Override behavior of MSBuild generator to automatically build the shader
        set_property(SOURCE ${FILE} PROPERTY VS_TOOL_OVERRIDE "None")

        # create a target for this shader
        set(SHADER_TARGET ${TARGET_ADD_SHADERS_TARGET}_${FILE_WE})
        add_custom_target(${SHADER_TARGET}
            DEPENDS ${FX_HEADER}
            SOURCES ${FILE})

        # Make the passed in target dependent on this shader
        add_dependencies(${TARGET_ADD_SHADERS_TARGET} ${SHADER_TARGET})
    endforeach(FILE)

    # Add the path to the shader header to the include path for the passed in target
    target_include_directories(${TARGET_ADD_SHADERS_TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/compiledshaders)

endfunction()