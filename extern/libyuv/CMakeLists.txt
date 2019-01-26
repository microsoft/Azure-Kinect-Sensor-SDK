if (NOT TARGET yuv)
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        add_compile_options("/D_CRT_SECURE_NO_WARNINGS")
    endif()

    add_subdirectory(src EXCLUDE_FROM_ALL)

    target_include_directories(yuv PUBLIC
        src
        src/include
    )
else()
    message(STATUS "yuv is already a target. Skipping adding it twice")
endif()

add_library(libyuv::libyuv ALIAS yuv)
