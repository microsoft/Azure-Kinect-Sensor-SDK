# Set a default build type if none was specified
# Taken from kitware's blog
# https://blog.kitware.com/cmake-and-the-default-build-type/
# Note: The CMake Windows-MSVC.cmake file, will set the build type to Debug by
#       default. So this will only work on builds with clang or gcc
set(default_build_type "RelWithDebInfo")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the build type")
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()