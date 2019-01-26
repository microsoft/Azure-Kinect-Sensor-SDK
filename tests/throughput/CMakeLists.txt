add_executable(throughput_perf throughput_perf.cpp)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    
target_link_libraries(throughput_perf PRIVATE 
    azure::aziotsharedutil
    gtest::gtest
    k4a::k4a
    k4ainternal::logging
    k4ainternal::utcommon)

k4a_add_tests(TARGET throughput_perf HARDWARE_REQUIRED TEST_TYPE PERF)
