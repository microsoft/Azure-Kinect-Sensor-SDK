function(determine_target_arch OUTPUT_VARIABLE)
    if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        if("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
            set(ARCH "i686")
        elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "x64")
            set(ARCH "x86_64")
        elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM")
            set(ARCH "arm")
        elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM64")
            set(ARCH "arm64")
        else()
            message(FATAL_ERROR "Unrecognized architecture ${MSVC_C_ARCHITECTURE_ID} from ${CMAKE_C_COMPILER}")
        endif()
  elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
        if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
            set(CMD_ARGS "-target" "${CMAKE_C_COMPILER_TARGET}" "-dumpmachine")
        else()
            set(CMD_ARGS "-dumpmachine")
        endif()
        execute_process(
            COMMAND 
                ${CMAKE_C_COMPILER} ${CMD_ARGS}
            RESULT_VARIABLE
                RESULT
            OUTPUT_VARIABLE
                ARCH
            ERROR_QUIET)
        if(RESULT)
            message(FATAL_ERROR "Failed to determine target architecture triplet: ${RESULT}")
        endif()
        string(REGEX MATCH "([^-]+).*" ARCH_MATCH ${ARCH})
        if(NOT CMAKE_MATCH_1 OR NOT ARCH_MATCH)
            message(FATAL_ERROR "Failed to match the target architecture triplet: ${ARCH}")
        endif()
        set(ARCH ${CMAKE_MATCH_1})

        if("${ARCH}" STREQUAL "x86_64")
            # Do nothing
        elseif("${ARCH}" STREQUAL "arm64")
            # Do nothing
        elseif("${ARCH}" STREQUAL "aarch64")
            set(ARCH "arm64")
        elseif("${ARCH}" STREQUAL "i686")
            # Do nothing
        elseif("${ARCH}" STREQUAL "i386")
            # Do nothing
        else()
            message(FATAL_ERROR "Unrecognized architecture ${ARCH} from ${CMAKE_C_COMPILER}")
        endif()
    else()
        message(FATAL_ERROR "Unrecognized Compiler ${CMAKE_C_COMPILER_ID}")
    endif()
    message(STATUS "Target architecture - ${ARCH}")
    set(${OUTPUT_VARIABLE} ${ARCH} PARENT_SCOPE)
endfunction()
