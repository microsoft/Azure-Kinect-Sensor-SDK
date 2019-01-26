add_executable(transformation_ut transformation.cpp)

target_link_libraries(transformation_ut PRIVATE
    azure::aziotsharedutil
    gtest::gtest
    k4ainternal::transformation
    k4ainternal::utcommon
    k4a::k4a)

k4a_add_tests(TARGET transformation_ut TEST_TYPE UNIT)
