add_executable(transformation_example 
    main.cpp
    transformation_helpers.cpp
    transformation_helpers.h)

target_link_libraries(transformation_example PRIVATE 
    k4a::k4a)