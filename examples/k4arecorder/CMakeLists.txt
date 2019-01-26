add_executable(k4arecorder main.cpp recorder.cpp)

target_link_libraries(k4arecorder PRIVATE
    k4a::k4a
    k4a::k4arecord
    )
