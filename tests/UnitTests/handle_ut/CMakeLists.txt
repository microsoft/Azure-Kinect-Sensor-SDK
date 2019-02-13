add_executable(handle_ut handle_ut.cpp handle2_ut.cpp)

target_link_libraries(handle_ut PRIVATE
    k4ainternal::utcommon
    )

k4a_add_tests(TARGET handle_ut TEST_TYPE UNIT)
