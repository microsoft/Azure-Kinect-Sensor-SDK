find_package(OpenCV)

add_executable(green_screen main.cpp)
target_link_libraries(green_screen PRIVATE k4a::k4a ${OpenCV_LIBS})
