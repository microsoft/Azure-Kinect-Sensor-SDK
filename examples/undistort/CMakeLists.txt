add_executable(undistort
    main.cpp)

target_link_libraries(undistort PRIVATE 
    k4a::k4a)