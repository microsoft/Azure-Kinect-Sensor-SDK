if("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
  add_library(LibUSB INTERFACE IMPORTED GLOBAL)
  target_include_directories(LibUSB INTERFACE "/opt/homebrew/include/libusb-1.0")
  target_link_libraries(LibUSB INTERFACE "/opt/homebrew/lib/libusb-1.0.dylib")
  add_library(LibUSB::LibUSB ALIAS LibUSB)

else()
  add_library(
    LibUSB STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/core.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/descriptor.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/hotplug.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/io.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/strerror.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/sync.c")

  target_include_directories(LibUSB
                             PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/libusb)
  target_include_directories(LibUSB
                             PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os)

  add_library(LibUSB::LibUSB ALIAS LibUSB)

  if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(LibUSB PUBLIC "-Wno-zero-length-array")
  endif()

  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    target_sources(
      LibUSB
      PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/poll_windows.c"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/threads_windows.c"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/windows_nt_common.c"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/windows_usbdk.c"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/windows_winusb.c")

    target_include_directories(LibUSB
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/msvc)

    target_compile_definitions(
      LibUSB PRIVATE "_LIB" "_CRT_SECURE_NO_WARNINGS" "WINVER=0x0501"
                     "_WIN32_WINNT=0x0501")

    target_compile_options(LibUSB PRIVATE "/source-charset:utf-8")

  elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    target_sources(
      LibUSB
      PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/poll_posix.c"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/threads_posix.c"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/linux_usbfs.c"
              "${CMAKE_CURRENT_SOURCE_DIR}/src/libusb/os/linux_udev.c")

    target_include_directories(LibUSB PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)

    target_link_libraries(LibUSB PUBLIC "udev")

  else()
    message(FATAL_ERROR "Unknown CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")
  endif()
endif()
