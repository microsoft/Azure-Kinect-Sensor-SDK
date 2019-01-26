add_executable(enumerate_devices main.c)
target_link_libraries(enumerate_devices PRIVATE k4a::k4a)