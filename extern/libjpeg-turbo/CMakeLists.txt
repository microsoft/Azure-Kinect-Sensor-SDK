if (NOT TARGET turbojpeg)
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_CRT_SECURE_NO_WARNINGS")
    endif()

    set(WITH_CRT_DLL ON CACHE BOOL "Force libjpeg to dynamically link CRT" FORCE)
    set(ENABLE_STATIC ON CACHE BOOL "Force libjpeg to enable static lib" FORCE)

    include(CheckLanguage)
    check_language(ASM_NASM)
    if (CMAKE_ASM_NASM_COMPILER)
        enable_language(ASM_NASM)
        option(WITH_SIMD "" ON)
    else()
        message(WARNING "NASM assembler not found - libjpeg-turbo performance may suffer")
        option(WITH_SIMD "" OFF)
    endif()

    set(CMAKE_INSTALL_DOCDIR ${CMAKE_CURRENT_BINARY_DIR}/install)

    # Stop libjpeg-turbo from overwritting the CMAKE_INSTALL_PREFIX DEFAULT
    set(TEMP_CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT ${CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT})
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT NO)
    add_subdirectory(src EXCLUDE_FROM_ALL)
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT ${TEMP_CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT})

    target_include_directories(turbojpeg-static PUBLIC
        src
    )
else()
    message(STATUS "turbojpeg is already a target. Skipping adding it twice")
endif()

add_library(libjpeg-turbo::libjpeg-turbo ALIAS turbojpeg-static)
