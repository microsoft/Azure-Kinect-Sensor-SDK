add_executable(streaming_samples main.c)
target_link_libraries(streaming_samples PRIVATE k4a::k4a
    azure::aziotsharedutil
    )