if (NOT TARGET cJSON)
    set(cjson_SOURCE_DIR src)

    add_library(cJSON STATIC
            ${cjson_SOURCE_DIR}/cJSON.c
            )

    # prevent cJSON.h from exporting its API's
    target_compile_definitions(cJSON PRIVATE -DCJSON_HIDE_SYMBOLS)

    # Consumers should #include <cJSON/cJSON.h>
    target_include_directories(cJSON PUBLIC
        ${cjson_SOURCE_DIR}/)

    # provide a path to cJSON.h which we just moved above
    target_include_directories(cJSON PRIVATE
        ${cjson_SOURCE_DIR}/cJSON)


else()
    message(STATUS "cJSON is already a target. Skipping adding it twice")
endif()

add_library(cJSON::cJSON ALIAS cJSON)