find_package(OpenCV)

add_executable(greenscreen main.cpp)
target_link_libraries(greenscreen PRIVATE k4a::k4a ${OpenCV_LIBS})
