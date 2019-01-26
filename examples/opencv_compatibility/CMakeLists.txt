add_executable(opencv_example
    main.cpp)

target_link_libraries(opencv_example PRIVATE
    k4a::k4a)
