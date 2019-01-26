if (NOT TARGET matroska)
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        # Turn off all warnings for MSVC
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
    endif()

    # This is a hack to trick libmatroska into thinking there is an installed
    # version of ebml. In reality, ebml is already a target since its source is
    # part of the build graph.
    set(Ebml_DIR "${CMAKE_CURRENT_LIST_DIR}")

    add_subdirectory(src EXCLUDE_FROM_ALL)
else()
    message(STATUS "matroska is already a target. Skipping adding it twice")
endif()

add_library(matroska::matroska ALIAS matroska)
