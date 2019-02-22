# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

find_package(Git REQUIRED QUIET)
if (NOT Git_FOUND)
    message(FATAL_ERROR "Unable to find git, which is needed for versioning")
endif()

function(get_git_dir DIRECTORY OUTPUT_VAR)
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} rev-parse --git-dir
        WORKING_DIRECTORY
            ${DIRECTORY}
        RESULT_VARIABLE
            GIT_DIR_RESULT
        OUTPUT_VARIABLE
            GIT_DIR_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (GIT_DIR_OUTPUT)
        set(${OUTPUT_VAR} ${GIT_DIR_OUTPUT} PARENT_SCOPE)
    endif()
endfunction()
