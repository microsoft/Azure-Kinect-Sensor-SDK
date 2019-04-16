# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# define the k4a_add_test function which is used for registering tests
include(k4aTest)

add_subdirectory(Calibration)
add_subdirectory(CaptureSync)
add_subdirectory(ColorTests)
add_subdirectory(DepthTests)
add_subdirectory(example)
add_subdirectory(ExternLibraries)
add_subdirectory(FirmwareTests)
add_subdirectory(logging)
add_subdirectory(IMUTests)
add_subdirectory(multidevice)
add_subdirectory(projections)
add_subdirectory(RecordTests)
add_subdirectory(TestUtil)
add_subdirectory(Transformation)
add_subdirectory(throughput)
add_subdirectory(UnitTests)
add_subdirectory(Utilities)
