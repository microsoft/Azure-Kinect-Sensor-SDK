if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    if (NOT TARGET project_libuvc)
        set(CMAKE_BUILD_TARGET Static)

        include(ExternalProject)
        ExternalProject_Add(project_libuvc
            GIT_REPOSITORY    https://github.com/ktossell/libuvc.git
            GIT_TAG           "v0.0.6"
            PREFIX            "${CMAKE_CURRENT_BINARY_DIR}"
            BUILD_BYPRODUCTS  ${CMAKE_CURRENT_BINARY_DIR}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/libuvc.a
            CMAKE_ARGS        -DCMAKE_BUILD_TARGET=Static -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR} -DCMAKE_LIBRARY_ARCHITECTURE=${CMAKE_LIBRARY_ARCHITECTURE} -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE
            PATCH_COMMAND     patch -s -N -p3 < ${CMAKE_CURRENT_SOURCE_DIR}/libuvc_metadata.patch || True
        )

        ExternalProject_Get_Property(project_libuvc install_dir)

        file(MAKE_DIRECTORY ${install_dir}/include)

        add_library(libuvc::libuvc STATIC IMPORTED GLOBAL)
        add_dependencies(libuvc::libuvc
            project_libuvc
            LibUSB::LibUSB)
        set_target_properties(libuvc::libuvc PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES ${install_dir}/include
            IMPORTED_LOCATION ${install_dir}/lib/${CMAKE_LIBRARY_ARCHITECTURE}/libuvc.a)

    else()
        message(STATUS "libuvc is already a target. Skipping adding it twice")
    endif()
endif()