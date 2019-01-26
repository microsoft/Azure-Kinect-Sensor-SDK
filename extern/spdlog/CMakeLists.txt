if (NOT TARGET spdlog)
    add_subdirectory(src EXCLUDE_FROM_ALL)
else()
    message(STATUS "spdlog is already a target. Skipping adding it twice")
endif()

add_library(spdlog::spdlog ALIAS spdlog)
