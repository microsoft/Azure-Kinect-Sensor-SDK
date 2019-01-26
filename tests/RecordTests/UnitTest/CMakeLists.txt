add_executable(record_ut record_ut.cpp)
add_executable(playback_ut playback_ut.cpp)

target_link_libraries(record_ut PRIVATE
    k4ainternal::utcommon
    k4ainternal::record
)

target_link_libraries(playback_ut PRIVATE
    k4ainternal::utcommon
    k4ainternal::playback
    k4a::k4arecord
)
configure_file("TestData/playback_test_input.mkv" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/TestData/playback_test_input.mkv" COPYONLY)

# Include the PUBLIC and INTERFACE directories specified by k4ainternal::record
target_include_directories(record_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::record,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(playback_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::playback,INTERFACE_INCLUDE_DIRECTORIES>)

k4a_add_tests(TARGET record_ut TEST_TYPE UNIT)
k4a_add_tests(TARGET playback_ut TEST_TYPE UNIT)
