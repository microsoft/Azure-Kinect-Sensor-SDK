if (NOT TARGET ebml)
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        # Turn off all warnings for MSVC
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        string(REGEX REPLACE "/W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
    endif()

    add_subdirectory(src EXCLUDE_FROM_ALL)
else()
    message(STATUS "ebml is already a target. Skipping adding it twice")
endif()

add_library(ebml::ebml ALIAS ebml)