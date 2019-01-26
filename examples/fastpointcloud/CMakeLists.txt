add_executable(fastpointcloud
    main.cpp)

target_link_libraries(fastpointcloud PRIVATE 
    k4a::k4a)