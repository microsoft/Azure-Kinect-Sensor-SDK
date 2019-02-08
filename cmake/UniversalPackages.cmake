find_program(
    AZURE_CLI
    NAMES
        "az"
        "az.cmd"
    DOC
        "Path to Azure CLI tool")

if (NOT AZURE_CLI)
    message(WARNING "Azure CLI not installed. Depth Engine plugin will not be automatically downloaded")
    message("You can find instructions to install Azure CLI here: https://docs.microsoft.com/en-us/cli/azure/install-azure-cli?view=azure-cli-latest")

    return()
endif()

if (NOT AZURE_CLI_AZURE_DEVOPS_ADDED)
    execute_process(
        COMMAND
            ${AZURE_CLI} extension add --name azure-devops
        RESULT_VARIABLE
            AZ_EXTENSION_ADD_RESULT
        OUTPUT_VARIABLE
            AZ_EXTENSION_ADD_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (NOT (${AZ_EXTENSION_ADD_RESULT} EQUAL 0))
        message(FATAL_ERROR "'az extension add --name azure-devops' failed: ${AZ_EXTENSION_ADD_OUTPUT}")
    endif()
    set(AZURE_CLI_AZURE_DEVOPS_ADDED ON CACHE BOOL "Azure CLI tool added azure-devops extension")
endif()


set(UNIVERSAL_PACKAGE_CACHE_DIR ${PROJECT_BINARY_DIR}/universalpkgs)

function(download_universal_package)
    cmake_parse_arguments(
        DL_UNI_PKG                                   # Prefix
        ""                                           # Options
        "ORGANIZATION;FEED;NAME;VERSION;OUTPUT_PATH" # One value keywords
        ""                                           # Multi value keywords
        ${ARGN})                                     # args...

    if (NOT DL_UNI_PKG_ORGANIZATION)
        message(FATAL_ERROR "No ORGANIZATION supplied to download_universal_package")
    endif()
    if (NOT DL_UNI_PKG_FEED)
        message(FATAL_ERROR "No FEED supplied to download_universal_package")
    endif()
    if (NOT DL_UNI_PKG_NAME)
        message(FATAL_ERROR "No NAME supplied to download_universal_package")
    endif()
    if (NOT DL_UNI_PKG_VERSION)
        message(FATAL_ERROR "No VERSION supplied to download_universal_package")
    endif()
    if (NOT DL_UNI_PKG_OUTPUT_PATH)
        message(FATAL_ERROR "No OUTPUT_PATH supplied to download_universal_package")
    endif()

    set(OUTPUT_PATH ${UNIVERSAL_PACKAGE_CACHE_DIR}/${DL_UNI_PKG_FEED}/${DL_UNI_PKG_NAME}/${DL_UNI_PKG_VERSION})
    set(TEMP_OUTPUT_PATH ${OUTPUT_PATH}_TEMP)

    if (NOT EXISTS ${OUTPUT_PATH})

        file(REMOVE_RECURSE ${TEMP_OUTPUT_PATH})

        execute_process(
            COMMAND
                ${AZURE_CLI} artifacts universal download --organization ${DL_UNI_PKG_ORGANIZATION} --feed ${DL_UNI_PKG_FEED} --name ${DL_UNI_PKG_NAME} --version ${DL_UNI_PKG_VERSION} --path ${TEMP_OUTPUT_PATH} --verbose
            RESULT_VARIABLE
                AZ_UNI_PKG_DOWNLOAD_RESULT
            OUTPUT_VARIABLE
                AZ_UNI_PKG_DOWNLOAD_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )


        if (${AZ_UNI_PKG_DOWNLOAD_RESULT} EQUAL 0)
            file(RENAME ${TEMP_OUTPUT_PATH} ${OUTPUT_PATH})
        else()
            message(FATAL_ERROR "azure universal download failed: ${AZ_UNI_PKG_DOWNLOAD_OUTPUT}")
        endif()

    endif()
    set(${DL_UNI_PKG_OUTPUT_PATH} ${OUTPUT_PATH} PARENT_SCOPE)
endfunction()