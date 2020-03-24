# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

add_executable(record_ut record_ut.cpp)
add_executable(playback_ut playback_ut.cpp test_helpers.cpp sample_recordings.cpp)
add_executable(custom_track_ut custom_track_ut.cpp test_helpers.cpp sample_recordings.cpp)
add_executable(playback_perf playback_perf.cpp test_helpers.cpp)

target_link_libraries(record_ut PRIVATE
    k4ainternal::utcommon
    k4ainternal::record
    k4a::k4arecord
)

target_link_libraries(playback_ut PRIVATE
    k4ainternal::utcommon
    k4ainternal::playback
    k4a::k4arecord
)

target_link_libraries(playback_perf PRIVATE
    k4ainternal::utcommon
    k4ainternal::playback
    k4a::k4arecord
)

target_link_libraries(custom_track_ut PRIVATE
    k4ainternal::utcommon
    k4ainternal::record
    k4a::k4arecord
)

# Include the PUBLIC and INTERFACE directories specified by k4ainternal::record
target_include_directories(record_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::record,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(playback_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::playback,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(playback_perf PRIVATE $<TARGET_PROPERTY:k4ainternal::playback,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(custom_track_ut PRIVATE $<TARGET_PROPERTY:k4ainternal::playback,INTERFACE_INCLUDE_DIRECTORIES>)

k4a_add_tests(TARGET record_ut TEST_TYPE UNIT)
k4a_add_tests(TARGET playback_ut TEST_TYPE UNIT)
k4a_add_tests(TARGET custom_track_ut TEST_TYPE UNIT)
