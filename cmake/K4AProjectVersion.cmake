find_package(Git REQUIRED QUIET)
if (NOT Git_FOUND)
    message(FATAL_ERROR "Unable to find git, which is needed for versioning")
endif()

# string(JOIN ...) was added in CMake 3.11 and thus can not be used.
# string_join was written to mimic string(JOIN ...) functionality and
# interface.
function(string_join GLUE OUTPUT VALUES)
    set(_TEMP_VALUES ${VALUES} ${ARGN})
    string(REPLACE ";" "${GLUE}" _TEMP_STR "${_TEMP_VALUES}")
    set(${OUTPUT} "${_TEMP_STR}" PARENT_SCOPE)
endfunction()

# When building in azure pipelines, the pipelines checkout a certain commit and
# thus no branch name is provided. To deal with this, we use the branch name
# provided as an enviromental variable.
#
# If we ever use a different CI system, this logic may need to be expanded
# there as well.
function(get_azure_pipelines_git_branch OUTPUT_VAR)
    if (DEFINED ENV{BUILD_SOURCEBRANCH})
        set(BRANCH_NAME $ENV{BUILD_SOURCEBRANCH})
        if (${BRANCH_NAME} MATCHES "refs/heads/.+")
            # This is to work around azure pipelines agent issue 838
            # https://github.com/Microsoft/azure-pipelines-agent/issues/838
            string(REPLACE "refs/heads/" "" BRANCH_NAME "${BRANCH_NAME}")
            set(${OUTPUT_VAR} ${BRANCH_NAME} PARENT_SCOPE)
        endif()
    endif()
endfunction()

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


function(get_git_branch GIT_REPO_DIR OUTPUT_VAR)
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY
            ${GIT_REPO_DIR}
        RESULT_VARIABLE
            GIT_BRANCH_NAME_RESULT
        OUTPUT_VARIABLE
            GIT_BRANCH_NAME_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT (${GIT_BRANCH_NAME_RESULT} EQUAL 0))
        message(FATAL_ERROR "git branch name failed: ${GIT_BRANCH_NAME_OUTPUT}")
    endif()

    set(${OUTPUT_VAR} ${GIT_BRANCH_NAME_OUTPUT} PARENT_SCOPE)
endfunction()

function(get_git_short_hash GIT_REPO_DIR OUTPUT_VAR)
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY
            ${GIT_REPO_DIR}
        RESULT_VARIABLE
            GIT_SHORT_HASH_RESULT
        OUTPUT_VARIABLE
            GIT_SHORT_HASH_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT (${GIT_SHORT_HASH_RESULT} EQUAL 0))
        message(FATAL_ERROR "git rev-parse failed: ${GIT_SHORT_HASH_OUTPUT}")
    endif()

    set(${OUTPUT_VAR} ${GIT_SHORT_HASH_OUTPUT} PARENT_SCOPE)
endfunction()

function(get_git_commit_count GIT_REPO_DIR PARENT_BRANCH OUTPUT_VAR)
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} rev-list --count HEAD ^origin/${PARENT_BRANCH}
        WORKING_DIRECTORY
            ${GIT_REPO_DIR}
        RESULT_VARIABLE
            GIT_COMMIT_COUNT_RESULT
        OUTPUT_VARIABLE
            GIT_COMMIT_COUNT_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT (${GIT_COMMIT_COUNT_RESULT} EQUAL 0))
        message(
            FATAL_ERROR
            "git commit count failed: ${GIT_COMMIT_COUNT_OUTPUT}"
        )
    endif()

    set(${OUTPUT_VAR} ${GIT_COMMIT_COUNT_OUTPUT} PARENT_SCOPE)
endfunction()

function(get_git_is_dirty GIT_REPO_DIR OUTPUT_VAR)
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} status --porcelain
        WORKING_DIRECTORY
            ${GIT_REPO_DIR}
        RESULT_VARIABLE
            GIT_STATUS_RESULT
        OUTPUT_VARIABLE
            GIT_STATUS_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT (${GIT_STATUS_RESULT} EQUAL 0))
        message(FATAL_ERROR "git status failed: ${GIT_STATUS_OUTPUT}")
    endif()

    if ("${GIT_STATUS_OUTPUT}" STREQUAL "")
        set(${OUTPUT_VAR} NO PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} YES PARENT_SCOPE)
    endif()
endfunction()

function(get_git_tag GIT_REPO_DIR OUTPUT_VAR)
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} describe --exact-match HEAD
        WORKING_DIRECTORY
            ${GIT_REPO_DIR}
        RESULT_VARIABLE
            GIT_DESCRIBE_RESULT
        OUTPUT_VARIABLE
            GIT_DESCRIBE_OUTPUT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (${GIT_DESCRIBE_RESULT} EQUAL 0)
        set(${OUTPUT_VAR} ${GIT_DESCRIBE_OUTPUT} PARENT_SCOPE)
    endif()
endfunction()


function(validate_branch_version BRANCH_NAME VERSION)
    set(VALID_BRANCH_REGEX "^(hotfix|release)/([0-9]+\\.[0-9]+\\.[0-9]+)")
    if (NOT (${BRANCH_NAME} MATCHES ${VALID_BRANCH_REGEX}))
        message(
            FATAL_ERROR
            "branches named hotfix/* or release/* must be versioned"
        )
    endif()
    if (NOT ("${VERSION}" STREQUAL "${CMAKE_MATCH_2}"))
        message(
            FATAL_ERROR
            "Branch name ${BRANCH_NAME} implied version ${CMAKE_MATCH_2} \
            doesn't match version.txt version ${VERSION}"
        )
    endif()
endfunction()

if ("${CMAKE_GENERATOR}" MATCHES "[A-Za-z] Makesfiles*")
    # Makefiles are unable to run any commands before the check to re-run cmake.
    # Because of this, after the first configure, the version variables in
    # cmake will never be updated. The code will still build properly, but all
    # subseqeunt versions may be wrong.
    message(
        WARNING
        "Makefiles may not generate the proper version. We recommend using \
        Ninja instead"
    )
endif()

set(SEMVER_REGEX "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
set(VERSION_TXT "${K4A_SOURCE_DIR}/version.txt")
file(STRINGS
    ${VERSION_TXT}
    VERSION_STR
    REGEX
        ${SEMVER_REGEX}
    LIMIT_COUNT
        1
)

if (NOT VERSION_STR)
    message(FATAL_ERROR "${VERSION_TXT} could not be read")
endif()

string(REGEX MATCH ${SEMVER_REGEX} VERSION_MATCH ${VERSION_STR})

if (NOT VERSION_MATCH)
    message(FATAL_ERROR "Contents of ${VERSION_TXT} are not a valid version")
endif()

set(K4A_VERSION_MAJOR ${CMAKE_MATCH_1})
set(K4A_VERSION_MINOR ${CMAKE_MATCH_2})
set(K4A_VERSION_PATCH ${CMAKE_MATCH_3})
set(K4A_VERSION_PRERELEASE)
set(K4A_VERSION_BUILDMETADATA)

# If not a git dir, just use version.txt
get_git_dir(${K4A_SOURCE_DIR} GIT_DIR)

if (GIT_DIR)
    get_azure_pipelines_git_branch(GIT_BRANCH)
    if (NOT GIT_BRANCH)
        get_git_branch(${K4A_SOURCE_DIR} GIT_BRANCH)
    endif()
    get_git_tag(${K4A_SOURCE_DIR} GIT_CURRENT_TAG)

    string(REGEX MATCH "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([0-9a-zA-Z\\-]+))?(\\+([0-9a-zA-Z\\-\\.]+))?$" GIT_TAG_IS_VERSION "${GIT_CURRENT_TAG}")
    if (GIT_TAG_IS_VERSION)
        # If this exact commit is tagged, verify the tagged major, minor, revision
        # matches the version.txt and use the tag.
        # If dirty, add dirty to build metadata
        if (NOT ("${VERSION_STR}" STREQUAL "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}"))
            message(FATAL_ERROR "git tag ${GIT_CURRENT_TAG} version mismatch with version.txt contents ${VERSION_STR}")
        endif()
        list(APPEND K4A_VERSION_PRERELEASE ${CMAKE_MATCH_5})
        list(APPEND K4A_VERSION_BUILDMETADATA ${CMAKE_MATCH_7})

        get_git_is_dirty(${K4A_SOURCE_DIR} GIT_IS_DIRTY)
        if (GIT_IS_DIRTY)
            list(APPEND K4A_VERSION_BUILDMETADATA "dirty")
        endif()

    # If no tag, then use the branch name to determine version
    elseif ("${GIT_BRANCH}" STREQUAL "master")
        # If master, just do major.minor.patch
        # If dirty, add dirty to build metadata
        get_git_is_dirty(${K4A_SOURCE_DIR} GIT_IS_DIRTY)
        if (GIT_IS_DIRTY)
            list(APPEND K4A_VERSION_BUILDMETADATA "dirty")
        endif()
    elseif("${GIT_BRANCH}" MATCHES "^release/*")
        # If release/* branch, check branch name version matches version.txt
        # Append "beta" to the prerelease string
        # Append number of commits from develop to prerelease string
        # If dirty, add dirty to build metadata
        validate_branch_version(
            ${GIT_BRANCH}
            "${K4A_VERSION_MAJOR}.${K4A_VERSION_MINOR}.${K4A_VERSION_PATCH}"
        )
        get_git_commit_count(${K4A_SOURCE_DIR} "develop" COMMIT_COUNT)
        set(K4A_VERSION_PRERELEASE "beta.${COMMIT_COUNT}")
        get_git_is_dirty(${K4A_SOURCE_DIR} GIT_IS_DIRTY)
        if (GIT_IS_DIRTY)
            list(APPEND K4A_VERSION_BUILDMETADATA "dirty")
        endif()
    elseif("${GIT_BRANCH}" MATCHES "^hotfix/*")
        # If hotfix/* branch, check branch name version matches version.txt
        # Append "rc" to the prerelease string
        # Append number of commits from master to prerelease string
        # If dirty, add dirty to build metadata
        validate_branch_version(
            ${GIT_BRANCH}
            "${K4A_VERSION_MAJOR}.${K4A_VERSION_MINOR}.${K4A_VERSION_PATCH}"
        )
        get_git_commit_count(${K4A_SOURCE_DIR} "master" COMMIT_COUNT)
        set(K4A_VERSION_PRERELEASE "rc.${COMMIT_COUNT}")
        get_git_is_dirty(${K4A_SOURCE_DIR} GIT_IS_DIRTY)
        if (GIT_IS_DIRTY)
            list(APPEND K4A_VERSION_BUILDMETADATA "dirty")
        endif()
    elseif("${GIT_BRANCH}" STREQUAL "develop")
        get_git_is_dirty(${K4A_SOURCE_DIR} GIT_IS_DIRTY)
        get_git_short_hash(${K4A_SOURCE_DIR} GIT_SHORT_HASH)
        set(K4A_VERSION_PRERELEASE "alpha")
        list(APPEND K4A_VERSION_BUILDMETADATA "git.${GIT_SHORT_HASH}")
        if (GIT_IS_DIRTY)
            list(APPEND K4A_VERSION_BUILDMETADATA "dirty")
        endif()

    # We are own an unknown branch with no tag. Assume developer or PR
    else()
        set(K4A_VERSION_PRERELEASE "privatebranch")
    endif()
endif()

set(
    K4A_VERSION
    "${K4A_VERSION_MAJOR}.${K4A_VERSION_MINOR}.${K4A_VERSION_PATCH}"
)

set(K4A_VERSION_STR ${K4A_VERSION})

if (K4A_VERSION_PRERELEASE)
    string(APPEND K4A_VERSION_STR "-${K4A_VERSION_PRERELEASE}")
endif()
if (K4A_VERSION_BUILDMETADATA)
    string_join("." K4A_VERSION_BUILDMETADATA_STR ${K4A_VERSION_BUILDMETADATA})
    string(APPEND K4A_VERSION_STR "+${K4A_VERSION_BUILDMETADATA_STR}")
endif()

# Below is a trick to verify the generated version has not changed since the
# last build.
#
# Because the version string is partially based on the results of git commands
# the complete version string can not be placed in a source file. Instead, we
# must run those git commands on each build. If the generated version string
# has changed, we must re-run cmake. Therefore, we must trick cmake into
# running the version check before determining if cmake needs to be
# reconfigured.
#
# We do this by using a custom target to generate a version.txt file which is
# the input to configure_file. The custom target has no dependencies and thus
# always runs. The custom target will only update the generated version.txt if
# its different than the calculated version. This will cause the configure_file
# to copy version.txt and have cmake re-configure.

if (CMAKE_SCRIPT_MODE_FILE)
    if (EXISTS "${K4A_VERSION_FILE}")
        file(READ ${K4A_VERSION_FILE} READ_IN_VERSION)
    else()
        set(READ_IN_VERSION "")
    endif()

    if (NOT ("${READ_IN_VERSION}" STREQUAL "${K4A_VERSION_STR}"))
        message(STATUS "Version is out of date")
        file(WRITE ${K4A_VERSION_FILE} ${K4A_VERSION_STR})
    endif()
else()
    set(K4A_VERSION_FILE ${CMAKE_CURRENT_BINARY_DIR}/version.txt)
    file(WRITE ${K4A_VERSION_FILE} ${K4A_VERSION_STR})

    # Create a target to check that the version.txt file is up to date
    # Tell cmake version.txt is a byproduct to add this k4a_git_version to the
    # build graph.
    add_custom_target(
        k4a_git_version
        BYPRODUCTS
            ${K4A_VERSION_FILE}
        COMMAND
            ${CMAKE_COMMAND}
            "-DK4A_SOURCE_DIR=${K4A_SOURCE_DIR}"
            "-DK4A_VERSION_FILE=${K4A_VERSION_FILE}"
            "-P" "${CMAKE_CURRENT_LIST_FILE}"
        COMMENT
            "Re-checking k4a version..."
        VERBATIM
        USES_TERMINAL
    )

    # This configure_file makes the cmake reconfigure dependent on version.txt
    configure_file(${K4A_VERSION_FILE} ${K4A_VERSION_FILE}.junk COPYONLY)

    message(STATUS "K4A Version: ${K4A_VERSION_STR}")
endif()
