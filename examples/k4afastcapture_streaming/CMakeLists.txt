find_package(Threads REQUIRED)

add_executable(fastcapture_streaming k4afastcapture.cpp main.cpp)
target_link_libraries(fastcapture_streaming PRIVATE 
    k4a::k4a
    "${CMAKE_THREAD_LIBS_INIT}"
    )
