if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    if (NOT TARGET libsoundio_static)
        # Some of the example stuff doesn't build under MSVC
        #
        set(BUILD_EXAMPLE_PROGRAMS OFF CACHE BOOL "Disable Soundio example projects")
        set(BUILD_TESTS OFF CACHE BOOL "Disable Soundio tests")
        set(SOUNDIO_STATIC_LIBNAME libsoundio_static)

        add_subdirectory(src EXCLUDE_FROM_ALL)

        target_include_directories(libsoundio_static PUBLIC
            src/soundio
        )

        target_compile_definitions(libsoundio_static PUBLIC "SOUNDIO_STATIC_LIBRARY")

        if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
            target_compile_definitions(libsoundio_static PRIVATE "_CRT_SECURE_NO_WARNINGS")
        endif()

        if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
            target_compile_options(libsoundio_static PRIVATE "-Wno-unused-variable")
        endif()

    else()
        message(STATUS "libsoundio_static is already a target. Skipping adding it twice")
    endif()

    add_library(libsoundio::libsoundio ALIAS libsoundio_static)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    find_library(LIBSOUNDIO_LIB soundio)
    add_library(libsoundio::libsoundio SHARED IMPORTED GLOBAL)
    set_property(TARGET libsoundio::libsoundio PROPERTY IMPORTED_LOCATION ${LIBSOUNDIO_LIB})
else()
    message(FATAL_ERROR "Unknown system name ${CMAKE_SYSTEM_NAME}")
endif()
