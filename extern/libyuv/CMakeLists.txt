if (NOT TARGET yuv)
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        add_compile_options("/D_CRT_SECURE_NO_WARNINGS")
    endif()

    set(CMAKE_IMPORT_LIBRARY_SUFFIX_CACHED ${CMAKE_IMPORT_LIBRARY_SUFFIX})
    unset(CMAKE_IMPORT_LIBRARY_SUFFIX)
    add_subdirectory(src EXCLUDE_FROM_ALL)
    set(CMAKE_IMPORT_LIBRARY_SUFFIX ${CMAKE_IMPORT_LIBRARY_SUFFIX_CACHED})

    target_include_directories(yuv PUBLIC
        src
        src/include
    )
else()
    message(STATUS "yuv is already a target. Skipping adding it twice")
endif()

add_library(libyuv::libyuv ALIAS yuv)
