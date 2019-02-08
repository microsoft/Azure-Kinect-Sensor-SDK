include(UniversalPackages)

if (COMMAND download_universal_package)
    set(DE_MAJOR_VER 1)
    set(DE_MINOR_VER 0)
    set(DE_PATCH_VER 1)

    set(DE_NAME "depthengine")

    download_universal_package(
        ORGANIZATION
            "https://dev.azure.com/microsoft"
        FEED
            "DepthEnginePlugin"
        NAME
            ${DE_NAME}
        VERSION
            "${DE_MAJOR_VER}.${DE_MINOR_VER}.${DE_PATCH_VER}"
        OUTPUT_PATH
            "DE_PATH"
    )


    if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
        if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
            if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
                set(DYNLIB_PATH ${DE_PATH}/windows/x86/debug/${DE_NAME}_${DE_MAJOR_VER}_${DE_MINOR_VER}.dll)
            else()
                set(DYNLIB_PATH ${DE_PATH}/windows/x86/release/${DE_NAME}_${DE_MAJOR_VER}_${DE_MINOR_VER}.dll)
            endif()
        elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
            if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
                set(DYNLIB_PATH ${DE_PATH}/windows/amd64/debug/${DE_NAME}_${DE_MAJOR_VER}_${DE_MINOR_VER}.dll)
            else()
                set(DYNLIB_PATH ${DE_PATH}/windows/amd64/release/${DE_NAME}_${DE_MAJOR_VER}_${DE_MINOR_VER}.dll)
            endif()
        else()
            message(FATAL_ERROR "Unsupported size of void ptr: ${CMAKE_SIZEOF_VOID_P}")
        endif()
    elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
        if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
            if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
                set(DYNLIB_PATH ${DE_PATH}/linux/x86/debug/lib${DE_NAME}.so.${DE_MAJOR_VER}.${DE_MINOR_VER})
            else()
                set(DYNLIB_PATH ${DE_PATH}/linux/x86/release/lib${DE_NAME}.so.${DE_MAJOR_VER}.${DE_MINOR_VER})
            endif()
        elseif("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
            if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
                set(DYNLIB_PATH ${DE_PATH}/linux/x86_64/debug/lib${DE_NAME}.so.${DE_MAJOR_VER}.${DE_MINOR_VER})
            else()
                set(DYNLIB_PATH ${DE_PATH}/linux/x86_64/release/lib${DE_NAME}.so.${DE_MAJOR_VER}.${DE_MINOR_VER})
            endif()
        else()
            message(FATAL_ERROR "Unsupported size of void ptr: ${CMAKE_SIZEOF_VOID_P}")
        endif()
    else()
        message(FATAL_ERROR "Unknown system name: ${CMAKE_SYSTEM_NAME}")
    endif()

    if (EXISTS ${DYNLIB_PATH})
        add_library(k4a::depthengine SHARED IMPORTED)
        set_target_properties(k4a::depthengine PROPERTIES
            IMPORTED_LOCATION "${DYNLIB_PATH}")

        if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
            include(CopyImportedBinary)
            copy_imported_binary(TARGET k4a::depthengine)
        endif()
    endif()

endif()