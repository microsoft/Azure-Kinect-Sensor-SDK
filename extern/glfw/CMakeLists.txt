if (NOT TARGET glfw)
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
        set(CLANG_DISABLE_ALL_WARNINGS "-Wno-everything")
        add_compile_options(${CLANG_DISABLE_ALL_WARNINGS})
    endif()

    # Set options for GLFW
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)

    add_subdirectory(src EXCLUDE_FROM_ALL)
else()
    message(STATUS "glfw is already a target. Skipping adding it twice")
endif()

add_library(glfw::glfw ALIAS glfw)
