//------------------------------------------------------------------------------
// <copyright file="DeviceFunctionTests.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using Microsoft.Azure.Kinect.Sensor.Test.StubGenerator;
using NUnit.Framework;
using System;

namespace Microsoft.Azure.Kinect.Sensor.UnitTests
{
    /// <summary>
    /// These tests validate the calling and marshaling behavior of the native functions in the C# wrapper
    /// 
    /// For any p/invoke function the tests should validate
    /// a) Input parameters are appropriately passed through
    /// b) Output parameters are appropriately returned
    /// c) All possible return values are appropriately handled (including errors translating to exceptions)
    /// d) The expected number of calls and calling pattern is provided (especially in the case of functions who have strict call ordering)
    /// 
    /// </summary>
    public class DeviceFunctionTests
    {
        private readonly StubbedModule NativeK4a;

        public DeviceFunctionTests()
        {
            NativeK4a = StubbedModule.Get("k4a");
            if (NativeK4a == null)
            {
                NativeInterface k4ainterface = NativeInterface.Create(
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.dll"),
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.h"));

                NativeK4a = StubbedModule.Create("k4a", k4ainterface);
            }
        }

        [SetUp]
        public void Setup()
        {
            // Don't hook the native allocator
            Microsoft.Azure.Kinect.Sensor.Allocator.Singleton.UseManagedAllocator = false;
        }

        // Helper function to implement basic open/close behavior
        private void SetOpenCloseImplementation()
        {
            NativeK4a.SetImplementation(@"

k4a_result_t k4a_set_debug_message_handler(
    k4a_logging_message_cb_t *message_cb,
    void *message_cb_context,
    k4a_log_level_t min_level)
{
    STUB_ASSERT(message_cb != NULL);

    return K4A_RESULT_SUCCEEDED;
}

k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle)
{
    STUB_ASSERT(index == 0);
    STUB_ASSERT(device_handle != NULL);

    // Assign back a fake value
    *device_handle = (k4a_device_t)0x1234ABCD; 

    return K4A_RESULT_SUCCEEDED;
}

void k4a_device_close(k4a_device_t device_handle)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
}");
        }

        [Test]
        public void GetInstalledCount()
        {
            NativeK4a.SetImplementation(@"

uint32_t k4a_device_get_installed_count()
{
    return 100;
}
");
            Assert.AreEqual(100, Device.GetInstalledCount());
        }

        [Test]
        public void DeviceOpenClose()
        {
            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle)
{{
    STUB_ASSERT(index == 5);
    STUB_ASSERT(device_handle != NULL);

    // Assign back a fake value
    *device_handle = (k4a_device_t)0x1234ABCD; 

    return K4A_RESULT_SUCCEEDED;
}}

void k4a_device_close(k4a_device_t device_handle)
{{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
}}

");
            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_device_open"));
            Assert.AreEqual(0, count.Calls("k4a_device_close"));

            using (Device.Open(5))
            {
                Assert.AreEqual(1, count.Calls("k4a_device_open"));
                Assert.AreEqual(0, count.Calls("k4a_device_close"));
            }

            Assert.AreEqual(1, count.Calls("k4a_device_open"));
            Assert.AreEqual(1, count.Calls("k4a_device_close"));
        }

        [Test]
        public void DeviceOpenFailure()
        {
            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle)
{{
    STUB_ASSERT(index == 5);
    STUB_ASSERT(device_handle != NULL);

    return K4A_RESULT_FAILED;
}}

void k4a_device_close(k4a_device_t device_handle)
{{
    STUB_FAIL(""close should not be called"");
}}

");

            _ = Assert.Throws(typeof(AzureKinectOpenDeviceException), () =>
            {
                using (Device.Open(5))
                {
                }
            });
        }

        private WeakReference CreateWithWeakReference<T>(System.Func<T> factory)
        {
            return new System.WeakReference(factory());
        }

        // Validate that garbage collection calls the handle close function by way of the finalizer
        [Test]
        public void DeviceGarbageCollection()
        {
            SetOpenCloseImplementation();
            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_device_open"));
            Assert.AreEqual(0, count.Calls("k4a_device_close"));

            System.WeakReference dev = CreateWithWeakReference(() =>
            {
                Device d = Device.Open(0);

                // The reference should still exist and we should have not seen close called
                Assert.AreEqual(1, count.Calls("k4a_device_open"));
                Assert.AreEqual(0, count.Calls("k4a_device_close"));

                return d;
            });
            // The reference to the Device object is no longer on the stack, and therefore is free to be garbage collected
            // At this point dev.IsAlive is likely to be true, but not garanteed to be

            // Force garbage collection
            System.GC.Collect(0, System.GCCollectionMode.Forced, true);
            System.GC.WaitForPendingFinalizers();


            Assert.AreEqual(false, dev.IsAlive);

            // k4a_device_close should have been called automatically 
            Assert.AreEqual(1, count.Calls("k4a_device_open"));
            Assert.AreEqual(1, count.Calls("k4a_device_close"));

        }

        /// <summary>
        /// Test the calls to k4a_device_get_serialnum
        /// </summary>
        /// <remarks>
        /// k4a_device_get_serialnum is expected to be called twice. Once to get the size of the serial number,
        /// and a second time to get the value of the serial number.
        /// </remarks>
        [Test]
        public void DeviceSerialNumber()
        {
            SetOpenCloseImplementation();
            NativeK4a.SetImplementation(@"
k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle,
                                                        char *serial_number,
                                                        size_t *serial_number_size)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);

    if (serial_number == NULL)
    {
        STUB_ASSERT(serial_number_size != NULL);
        STUB_ASSERT(*serial_number_size == 0);

        *serial_number_size = 5;
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
    else
    {
        STUB_ASSERT(serial_number_size != NULL);
        STUB_ASSERT(*serial_number_size == 5);

        strcpy_s(serial_number, *serial_number_size, ""1234"");
        return K4A_BUFFER_RESULT_SUCCEEDED;
    }
}
");
            CallCount count = NativeK4a.CountCalls();
            using (Device device = Device.Open(0))
            {
                Assert.AreEqual("1234", device.SerialNum);
                Assert.AreEqual(2, count.Calls("k4a_device_get_serialnum"));

                // Verify the serial number is cached
                Assert.AreEqual("1234", device.SerialNum);
                Assert.AreEqual(2, count.Calls("k4a_device_get_serialnum"));


                device.Dispose();
                Assert.Throws(typeof(ObjectDisposedException), () =>
                {
                    _ = device.SerialNum;
                });

            }
        }

        [Test]
        public void DeviceSerialNumberFailure()
        {
            SetOpenCloseImplementation();
            NativeK4a.SetImplementation(@"
k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle,
                                                        char *serial_number,
                                                        size_t *serial_number_size)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);

    if (serial_number == NULL)
    {
        STUB_ASSERT(serial_number_size != NULL);
        STUB_ASSERT(*serial_number_size == 0);
        
        *serial_number_size = 5;
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
    else
    {
        STUB_ASSERT(serial_number_size != NULL);
        STUB_ASSERT(*serial_number_size == 5);

        return K4A_BUFFER_RESULT_FAILED;
    }
}
");
            //Validate that we get exceptions from the second call to k4a_device_get_serialnum
            using (Device device = Device.Open(0))
            {
                _ = Assert.Throws<AzureKinectException>(() =>
                {
                    Assert.AreEqual("1234", device.SerialNum);
                });
            }

            NativeK4a.SetImplementation(@"
k4a_buffer_result_t k4a_device_get_serialnum(k4a_device_t device_handle,
                                                        char *serial_number,
                                                        size_t *serial_number_size)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);

    if (serial_number == NULL)
    {
        STUB_ASSERT(serial_number_size != NULL);
        STUB_ASSERT(*serial_number_size == 0);

        return K4A_BUFFER_RESULT_FAILED;
    }
    else
    {
        STUB_FAIL(""unexpected calling pattern"");
    }
}
");

            // Validate that we get exceptions from the first call to k4a_device_get_serialnum
            using (Device device = Device.Open(0))
            {
                Assert.Throws<InvalidOperationException>(() =>
                {
                    string sn = device.SerialNum;
                });
            }
        }


        [Test]
        public void DeviceGetRawCalibration()
        {
            SetOpenCloseImplementation();
            NativeK4a.SetImplementation(@"
k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle,
                                                              uint8_t  *data,
                                                              size_t *data_size)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);

    if (data == NULL)
    {
        STUB_ASSERT(data_size != NULL);
        STUB_ASSERT(*data_size == 0);

        *data_size = 1234;
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
    else
    {
        STUB_ASSERT(data_size != NULL);
        STUB_ASSERT(*data_size == 1234);

        for (int i = 0; i < *data_size; i++)
        {
            data[i] = (uint8_t)i;
        }

        return K4A_BUFFER_RESULT_SUCCEEDED;
    }
}
");
            using (Device device = Device.Open(0))
            {
                byte[] rawCalibration = device.GetRawCalibration();
                Assert.AreEqual(1234, rawCalibration.Length);
                for (int i = 0; i < rawCalibration.Length; i++)
                {
                    Assert.AreEqual((byte)i, rawCalibration[i]);
                }

                device.Dispose();

                Assert.Throws(typeof(ObjectDisposedException), () =>
                {
                    device.GetRawCalibration();
                });
            }
        }

        [Test]
        public void DeviceGetRawCalibrationFailure()
        {
            SetOpenCloseImplementation();


            NativeK4a.SetImplementation(@"
k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle,
                                                              uint8_t  *data,
                                                              size_t *data_size)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);

    if (data == NULL)
    {
        STUB_ASSERT(data_size != NULL);
        STUB_ASSERT(*data_size == 0);
        
        *data_size = 5;
        return K4A_BUFFER_RESULT_TOO_SMALL;
    }
    else
    {
        STUB_ASSERT(data_size != NULL);
        STUB_ASSERT(*data_size == 5);

        return K4A_BUFFER_RESULT_FAILED;
    }
}
");

            CallCount count = NativeK4a.CountCalls();
            //Validate that we get exceptions from the second call to k4a_device_get_serialnum
            using (Device device = Device.Open(0))
            {
                Assert.Throws<Microsoft.Azure.Kinect.Sensor.AzureKinectException>(() =>
                {
                    device.GetRawCalibration();
                });
            }
            Assert.AreEqual(2, count.Calls("k4a_device_get_raw_calibration"));

            NativeK4a.SetImplementation(@"
k4a_buffer_result_t k4a_device_get_raw_calibration(k4a_device_t device_handle,
                                                              uint8_t  *data,
                                                              size_t *data_size)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);

    if (data == NULL)
    {
        STUB_ASSERT(data_size != NULL);
        STUB_ASSERT(*data_size == 0);
        
        *data_size = 5;
        return K4A_BUFFER_RESULT_FAILED;
    }
    else
    {
        STUB_FAIL(""unexpected calling pattern"");
    }
}
");

            count = NativeK4a.CountCalls();
            //Validate that we get exceptions from the first call to k4a_device_get_raw_calibration
            using (Device device = Device.Open(0))
            {
                Assert.Throws<Microsoft.Azure.Kinect.Sensor.AzureKinectException>(() =>
                {
                    device.GetRawCalibration();
                });
            }
            Assert.AreEqual(1, count.Calls("k4a_device_get_raw_calibration"));
        }


        [Test]
        public void DeviceGetCalibration()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_get_calibration(k4a_device_t device_handle, k4a_depth_mode_t depth_mode, k4a_color_resolution_t color_resolution, k4a_calibration_t* calibration)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(depth_mode == K4A_DEPTH_MODE_NFOV_UNBINNED);
    STUB_ASSERT(color_resolution == K4A_COLOR_RESOLUTION_1440P);
    STUB_ASSERT(calibration != NULL);

    // Fill the structure with values we can verify (pick values to pass that are unique so that a swap of values will be detected)
    calibration->depth_mode = depth_mode;
    calibration->color_resolution = color_resolution;
    
    for (int i = 0; i < 9; i++)
        calibration->depth_camera_calibration.extrinsics.rotation[i] = (float)i * 1.2f;

    for (int i = 0; i < 3; i++)
        calibration->depth_camera_calibration.extrinsics.translation[i] = (float)i * 1.3f;

    calibration->depth_camera_calibration.intrinsics.type = K4A_CALIBRATION_LENS_DISTORTION_MODEL_BROWN_CONRADY;
    calibration->depth_camera_calibration.intrinsics.parameter_count = 81;
    for (int i = 0; i < 15; i++)
        calibration->depth_camera_calibration.intrinsics.parameters.v[i] = (float)i * 1.4f;    

    calibration->depth_camera_calibration.resolution_width = 91;
    calibration->depth_camera_calibration.resolution_height = 92;
    calibration->depth_camera_calibration.metric_radius = 1.23f;

    calibration->color_camera_calibration.resolution_width = 101;
    calibration->color_camera_calibration.resolution_height = 102;
    calibration->color_camera_calibration.metric_radius = 4.56f;

    return K4A_RESULT_SUCCEEDED;
}
");
            CallCount count = NativeK4a.CountCalls();
            using (Device device = Device.Open(0))
            {
                Calibration calibration = device.GetCalibration(DepthMode.NFOV_Unbinned, ColorResolution.R1440p);

                Assert.AreEqual(1, count.Calls("k4a_device_get_calibration"));

                Assert.AreEqual(DepthMode.NFOV_Unbinned, calibration.DepthMode);
                Assert.AreEqual(ColorResolution.R1440p, calibration.ColorResolution);

                for (int i = 0; i < 9; i++)
                {
                    Assert.AreEqual(i * 1.2f, calibration.DepthCameraCalibration.Extrinsics.Rotation[i]);
                }
                for (int i = 0; i < 3; i++)
                {
                    Assert.AreEqual(i * 1.3f, calibration.DepthCameraCalibration.Extrinsics.Translation[i]);
                }

                Assert.AreEqual(81, calibration.DepthCameraCalibration.Intrinsics.ParameterCount);
                Assert.AreEqual(CalibrationModelType.BrownConrady, calibration.DepthCameraCalibration.Intrinsics.Type);

                for (int i = 0; i < calibration.DepthCameraCalibration.Intrinsics.Parameters.Length; i++)
                {
                    Assert.AreEqual(i * 1.4f, calibration.DepthCameraCalibration.Intrinsics.Parameters[i]);
                }
                Assert.AreEqual(91, calibration.DepthCameraCalibration.ResolutionWidth);
                Assert.AreEqual(92, calibration.DepthCameraCalibration.ResolutionHeight);
                Assert.AreEqual(1.23f, calibration.DepthCameraCalibration.MetricRadius);

                Assert.AreEqual(101, calibration.ColorCameraCalibration.ResolutionWidth);
                Assert.AreEqual(102, calibration.ColorCameraCalibration.ResolutionHeight);
                Assert.AreEqual(4.56f, calibration.ColorCameraCalibration.MetricRadius);


                // GetCalibration with no arguments will throw if the device is not yet started
                Assert.Throws(typeof(AzureKinectException), () =>
                {
                    device.GetCalibration();
                });

                NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_start_cameras(
    k4a_device_t device_handle,
    const k4a_device_configuration_t * config
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(config != NULL);

    return K4A_RESULT_SUCCEEDED;
}

void k4a_device_stop_cameras(k4a_device_t device_handle)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
}
");

                device.StartCameras(new DeviceConfiguration
                {
                    ColorResolution = ColorResolution.R1440p,
                    DepthMode = DepthMode.NFOV_Unbinned
                });

                // Calibration should return correctly now
                Assert.IsNotNull(device.GetCalibration());

                device.StopCameras();

                // GetCalibration will fail again after the cameras are stopped
                Assert.Throws(typeof(AzureKinectException), () =>
                {
                    device.GetCalibration();
                });

                device.Dispose();

                Assert.Throws(typeof(ObjectDisposedException), () =>
                    {
                        device.GetCalibration(DepthMode.NFOV_Unbinned, ColorResolution.R1440p);
                    });

            }


        }

        [Test]
        public void DeviceGetCalibrationFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_get_calibration(k4a_device_t device_handle, k4a_depth_mode_t depth_mode, k4a_color_resolution_t color_resolution, k4a_calibration_t* calibration)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(depth_mode == K4A_DEPTH_MODE_NFOV_UNBINNED);
    STUB_ASSERT(color_resolution == K4A_COLOR_RESOLUTION_1440P);
    STUB_ASSERT(calibration != NULL);

    return K4A_RESULT_FAILED;
}
");
            using (Device device = Device.Open(0))
            {
                Assert.Throws(typeof(AzureKinectException), () =>
                    {
                        Calibration calibration = device.GetCalibration(DepthMode.NFOV_Unbinned, ColorResolution.R1440p);
                    });

            }

        }

        [Test]
        public void DeviceGetCapture()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle, k4a_capture_t* capture_handle, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(capture_handle != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    *capture_handle = (k4a_capture_t)0x0C001234;
    return K4A_WAIT_RESULT_SUCCEEDED;
}

void k4a_capture_release(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle == (k4a_capture_t)0x0C001234);
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));
                    using (Capture capture = device.GetCapture(System.TimeSpan.FromMilliseconds(2345)))
                    {
                        Assert.AreEqual(1, count.Calls("k4a_device_get_capture"));
                        Assert.AreEqual(0, count.Calls("k4a_capture_release"));
                    }

                    Assert.AreEqual(1, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(1, count.Calls("k4a_capture_release"));
                }
            }

            {
                CallCount count = NativeK4a.CountCalls();
                Capture capture;
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));

                    capture = device.GetCapture(System.TimeSpan.FromMilliseconds(2345));

                    Assert.AreEqual(1, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));

                    device.Dispose();
                    Assert.Throws(typeof(ObjectDisposedException), () =>
                    {
                        device.GetCapture();
                    });
                }

                capture.Dispose();
                Assert.AreEqual(1, count.Calls("k4a_device_get_capture"));
                Assert.AreEqual(1, count.Calls("k4a_capture_release"));
            }
        }

        [Test]
        public void DeviceGetCaptureTimeout()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle, k4a_capture_t* capture_handle, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(capture_handle != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    *capture_handle = (k4a_capture_t)0x0C001234;
    return K4A_WAIT_RESULT_TIMEOUT;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));

                    Assert.Throws(typeof(System.TimeoutException), () =>
                    {
                        using (Capture capture = device.GetCapture(System.TimeSpan.FromMilliseconds(2345)))
                        {

                        }
                    });

                    Assert.AreEqual(1, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));
                }
            }

            NativeK4a.SetImplementation(@"

k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle, k4a_capture_t* capture_handle, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(capture_handle != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    *capture_handle = (k4a_capture_t)0x0C001234;
    return K4A_WAIT_RESULT_SUCCEEDED;
}
");

        }

        [Test]
        public void DeviceGetCaptureFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle, k4a_capture_t* capture_handle, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(capture_handle != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    *capture_handle = (k4a_capture_t)0x0C001234;
    return K4A_WAIT_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));

                    Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                    {
                        using (Capture capture = device.GetCapture(System.TimeSpan.FromMilliseconds(2345)))
                        {

                        }
                    });

                    Assert.AreEqual(1, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));
                }
            }


            // Verify that if the native code returns success, but no handle, we properly throw an exception
            NativeK4a.SetImplementation(@"

k4a_wait_result_t k4a_device_get_capture(k4a_device_t device_handle, k4a_capture_t* capture_handle, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(capture_handle != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    *capture_handle = (k4a_capture_t)0;
    return K4A_WAIT_RESULT_SUCCEEDED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));

                    Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                    {
                        using (Capture capture = device.GetCapture(System.TimeSpan.FromMilliseconds(2345)))
                        {

                        }
                    });

                    Assert.AreEqual(1, count.Calls("k4a_device_get_capture"));
                    Assert.AreEqual(0, count.Calls("k4a_capture_release"));
                }
            }
        }



        [Test]
        public void DeviceSetColorControl()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_set_color_control(k4a_device_t device_handle, k4a_color_control_command_t command, k4a_color_control_mode_t mode, int32_t value)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(command == K4A_COLOR_CONTROL_POWERLINE_FREQUENCY);
    STUB_ASSERT(mode == K4A_COLOR_CONTROL_MODE_MANUAL);
    STUB_ASSERT(value == 2345);

    return K4A_RESULT_SUCCEEDED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_set_color_control"));
                    device.SetColorControl(ColorControlCommand.PowerlineFrequency, ColorControlMode.Manual, 2345);
                    Assert.AreEqual(1, count.Calls("k4a_device_set_color_control"));

                    device.Dispose();

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        device.SetColorControl(ColorControlCommand.PowerlineFrequency, ColorControlMode.Manual, 2345);
                    });
                }
            }
        }

        [Test]
        public void DeviceSetColorControlFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_set_color_control(k4a_device_t device_handle, k4a_color_control_command_t command, k4a_color_control_mode_t mode, int32_t value)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(command == K4A_COLOR_CONTROL_POWERLINE_FREQUENCY);
    STUB_ASSERT(mode == K4A_COLOR_CONTROL_MODE_MANUAL);
    STUB_ASSERT(value == 2345);

    return K4A_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_set_color_control"));
                    Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                    {
                        device.SetColorControl(ColorControlCommand.PowerlineFrequency, ColorControlMode.Manual, 2345);
                    });
                    Assert.AreEqual(1, count.Calls("k4a_device_set_color_control"));
                }
            }
        }


        [Test]
        public void DeviceGetColorControl()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_get_color_control(k4a_device_t device_handle, k4a_color_control_command_t command, k4a_color_control_mode_t *mode, int32_t *value)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(command == K4A_COLOR_CONTROL_POWERLINE_FREQUENCY);
    STUB_ASSERT(mode != NULL);
    STUB_ASSERT(value != NULL);

    *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
    *value = 2345;

    return K4A_RESULT_SUCCEEDED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_color_control"));
                    Assert.AreEqual(2345, device.GetColorControl(ColorControlCommand.PowerlineFrequency, out ColorControlMode mode));
                    Assert.AreEqual(ColorControlMode.Manual, mode);
                    Assert.AreEqual(1, count.Calls("k4a_device_get_color_control"));

                    Assert.AreEqual(2345, device.GetColorControl(ColorControlCommand.PowerlineFrequency));
                    Assert.AreEqual(2, count.Calls("k4a_device_get_color_control"));

                    device.Dispose();

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        device.GetColorControl(ColorControlCommand.PowerlineFrequency, out ColorControlMode mode2);
                    });

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        device.GetColorControl(ColorControlCommand.PowerlineFrequency);
                    });
                }
            }
        }

        [Test]
        public void DeviceGetColorControlFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_get_color_control(k4a_device_t device_handle, k4a_color_control_command_t command, k4a_color_control_mode_t *mode, int32_t *value)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(command == K4A_COLOR_CONTROL_POWERLINE_FREQUENCY);
    STUB_ASSERT(mode != NULL);
    STUB_ASSERT(value != NULL);

    *mode = K4A_COLOR_CONTROL_MODE_MANUAL;
    *value = 2345;

    return K4A_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_color_control"));
                    Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                    {
                        device.GetColorControl(ColorControlCommand.PowerlineFrequency, out ColorControlMode mode);
                    });
                    Assert.AreEqual(1, count.Calls("k4a_device_get_color_control"));
                }
            }
        }

        [Test]
        public void DeviceGetImuSample()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_wait_result_t k4a_device_get_imu_sample(k4a_device_t device_handle, k4a_imu_sample_t* imu_sample, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(imu_sample != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    STUB_ASSERT(imu_sample->temperature == 0.0f);

    imu_sample->temperature = 0.123f;

    imu_sample->acc_sample.v[0] = 0.0f;
    imu_sample->acc_sample.v[1] = 0.1f;
    imu_sample->acc_sample.v[2] = 0.2f;
    // 10 seconds
    imu_sample->acc_timestamp_usec = 10000000;

    imu_sample->gyro_sample.v[0] = 0.4f;
    imu_sample->gyro_sample.v[1] = 0.5f;
    imu_sample->gyro_sample.v[2] = 0.6f;

    imu_sample->gyro_timestamp_usec = 60000000;

    return K4A_WAIT_RESULT_SUCCEEDED;
}

");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_imu_sample"));
                    ImuSample sample = device.GetImuSample(System.TimeSpan.FromMilliseconds(2345));

                    Assert.AreEqual(0.123f, sample.Temperature);

                    Assert.AreEqual(0.0f, sample.AccelerometerSample.X);
                    Assert.AreEqual(0.1f, sample.AccelerometerSample.Y);
                    Assert.AreEqual(0.2f, sample.AccelerometerSample.Z);
                    Assert.AreEqual(TimeSpan.FromSeconds(10), sample.AccelerometerTimestamp);

                    Assert.AreEqual(0.4f, sample.GyroSample.X);
                    Assert.AreEqual(0.5f, sample.GyroSample.Y);
                    Assert.AreEqual(0.6f, sample.GyroSample.Z);
                    Assert.AreEqual(TimeSpan.FromMinutes(1), sample.GyroTimestamp);

                    Assert.AreEqual(1, count.Calls("k4a_device_get_imu_sample"));

                    device.Dispose();
                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        device.GetImuSample(System.TimeSpan.FromMilliseconds(2345));
                    });
                }
            }


        }

        [Test]
        public void DeviceGetImuSampleTimeout()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"

k4a_wait_result_t k4a_device_get_imu_sample(k4a_device_t device_handle, k4a_imu_sample_t* imu_sample, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(imu_sample != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    return K4A_WAIT_RESULT_TIMEOUT;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_imu_sample"));

                    Assert.Throws(typeof(System.TimeoutException), () =>
                    {
                        ImuSample sample = device.GetImuSample(System.TimeSpan.FromMilliseconds(2345));
                    });

                    Assert.AreEqual(1, count.Calls("k4a_device_get_imu_sample"));
                }
            }
        }

        [Test]
        public void DeviceGetImuSampleFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_wait_result_t k4a_device_get_imu_sample(k4a_device_t device_handle, k4a_imu_sample_t* imu_sample, int32_t timeout_in_ms)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(imu_sample != NULL);
    STUB_ASSERT(timeout_in_ms == 2345);

    return K4A_WAIT_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.AreEqual(0, count.Calls("k4a_device_get_imu_sample"));

                    Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                    {
                        ImuSample sample = device.GetImuSample(System.TimeSpan.FromMilliseconds(2345));
                    });

                    Assert.AreEqual(1, count.Calls("k4a_device_get_imu_sample"));
                }
            }
        }

        [Test]
        public void DeviceSyncJackConnected()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_get_sync_jack(
    k4a_device_t device_handle,
    bool * sync_in_jack_connected,
    bool * sync_out_jack_connected
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(sync_in_jack_connected != NULL);
    STUB_ASSERT(sync_out_jack_connected != NULL);
    
    *sync_in_jack_connected = true;
    *sync_out_jack_connected = false;

    return K4A_RESULT_SUCCEEDED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    // Assume that the sync state hasn't been cached by Device
                    // Unless the native API has a means of notifing the managed wrapper the
                    // state has changed, this should be true. We would expect every property read 
                    // to requery the state.
                    Assert.AreEqual(0, count.Calls("k4a_device_get_sync_jack"));

                    Assert.AreEqual(true, device.SyncInJackConnected);
                    Assert.AreEqual(1, count.Calls("k4a_device_get_sync_jack"));

                    Assert.AreEqual(false, device.SyncOutJackConnected);
                    Assert.AreEqual(2, count.Calls("k4a_device_get_sync_jack"));

                    NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_get_sync_jack(
    k4a_device_t device_handle,
    bool * sync_in_jack_connected,
    bool * sync_out_jack_connected
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(sync_in_jack_connected != NULL);
    STUB_ASSERT(sync_out_jack_connected != NULL);
    
    *sync_in_jack_connected = false;
    *sync_out_jack_connected = true;

    return K4A_RESULT_SUCCEEDED;
}
");
                    Assert.AreEqual(false, device.SyncInJackConnected);
                    Assert.AreEqual(3, count.Calls("k4a_device_get_sync_jack"));

                    Assert.AreEqual(true, device.SyncOutJackConnected);
                    Assert.AreEqual(4, count.Calls("k4a_device_get_sync_jack"));

                    device.Dispose();

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        _ = device.SyncInJackConnected;
                    });

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        _ = device.SyncOutJackConnected;
                    });
                }
            }
        }

        [Test]
        public void DeviceSyncJackConnectedFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_get_sync_jack(
    k4a_device_t device_handle,
    bool * sync_in_jack_connected,
    bool * sync_out_jack_connected
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(sync_in_jack_connected != NULL);
    STUB_ASSERT(sync_out_jack_connected != NULL);
    
    *sync_in_jack_connected = true;
    *sync_out_jack_connected = false;

    return K4A_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {

                    Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                    {
                        bool instate = device.SyncInJackConnected;
                    });

                    Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                    {
                        bool instate = device.SyncOutJackConnected;
                    });
                }
            }
        }

        [Test]
        public void DeviceGetVersion()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_get_version(
    k4a_device_t device_handle,
    k4a_hardware_version_t * version
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(version != NULL);
    
    version->rgb.major = 1;
    version->rgb.minor = 2;
    version->rgb.iteration = 3;

    version->depth.major = 4;
    version->depth.minor = 5;
    version->depth.iteration = 6;

    version->audio.major = 7;
    version->audio.minor = 8;
    version->audio.iteration = 9;

    version->depth_sensor.major = 10;
    version->depth_sensor.minor = 11;
    version->depth_sensor.iteration = 12;

    version->firmware_build = K4A_FIRMWARE_BUILD_DEBUG;
    version->firmware_signature = K4A_FIRMWARE_SIGNATURE_UNSIGNED;

    return K4A_RESULT_SUCCEEDED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    HardwareVersion version = device.Version;

                    Assert.AreEqual(new System.Version("1.2.3"), device.Version.RGB);
                    Assert.AreEqual(new System.Version("4.5.6"), device.Version.Depth);
                    Assert.AreEqual(new System.Version("7.8.9"), device.Version.Audio);
                    Assert.AreEqual(new System.Version("10.11.12"), device.Version.DepthSensor);
                    Assert.AreEqual(FirmwareBuild.Debug, device.Version.FirmwareBuild);
                    Assert.AreEqual(FirmwareSignature.NotSigned, device.Version.FirmwareSignature);

                    // Since the version numbers shouldn't change in the lifetime of a device,
                    // and callers may use the Version sub-properties many times, we should
                    // ensure that we cache the results of the marshalling.
                    Assert.AreEqual(1, count.Calls("k4a_device_get_version"));
                }

                using (Device device = Device.Open(0))
                {
                    HardwareVersion version = device.Version;

                    // A second device instance should not have the same cache
                    Assert.AreEqual(2, count.Calls("k4a_device_get_version"));

                    device.Dispose();

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        _ = device.Version;
                    });

                }
            }
        }

        [Test]
        public void DeviceGetVersionFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_get_version(
    k4a_device_t device_handle,
    k4a_hardware_version_t * version
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(version != NULL);

    return K4A_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                Assert.Throws(typeof(Microsoft.Azure.Kinect.Sensor.AzureKinectException), () =>
                {
                    using (Device device = Device.Open(0))
                    {
                        HardwareVersion version = device.Version;
                    }
                });

            }
        }

        [Test]
        public void DeviceStartCameras()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_start_cameras(
    k4a_device_t device_handle,
    const k4a_device_configuration_t * config
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(config != NULL);

    STUB_ASSERT(config->color_format == K4A_IMAGE_FORMAT_COLOR_MJPG);
    STUB_ASSERT(config->color_resolution == K4A_COLOR_RESOLUTION_OFF);
    STUB_ASSERT(config->depth_mode == K4A_DEPTH_MODE_OFF);
    STUB_ASSERT(config->camera_fps == K4A_FRAMES_PER_SECOND_30);
    STUB_ASSERT(config->synchronized_images_only == false);
    STUB_ASSERT(config->depth_delay_off_color_usec == 0);
    STUB_ASSERT(config->wired_sync_mode == K4A_WIRED_SYNC_MODE_STANDALONE);
    STUB_ASSERT(config->subordinate_delay_off_master_usec == 0);
    STUB_ASSERT(config->disable_streaming_indicator == false);
    

    return K4A_RESULT_SUCCEEDED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.Throws(typeof(System.ArgumentNullException), () =>
                    {
                        device.StartCameras(null);
                    });

                    // Verify default configuration
                    device.StartCameras(new DeviceConfiguration());
                    Assert.AreEqual(1, count.Calls("k4a_device_start_cameras"));

                    NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_start_cameras(
    k4a_device_t device_handle,
    const k4a_device_configuration_t * config
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(config != NULL);

    STUB_ASSERT(config->color_format == K4A_IMAGE_FORMAT_COLOR_BGRA32);
    STUB_ASSERT(config->color_resolution == K4A_COLOR_RESOLUTION_1080P);
    STUB_ASSERT(config->depth_mode == K4A_DEPTH_MODE_PASSIVE_IR);
    STUB_ASSERT(config->camera_fps == K4A_FRAMES_PER_SECOND_15);
    STUB_ASSERT(config->synchronized_images_only == true);
    STUB_ASSERT(config->depth_delay_off_color_usec == -1000000);
    STUB_ASSERT(config->wired_sync_mode == K4A_WIRED_SYNC_MODE_MASTER);
    STUB_ASSERT(config->subordinate_delay_off_master_usec == 500000);
    STUB_ASSERT(config->disable_streaming_indicator == true);
    
    return K4A_RESULT_SUCCEEDED;
}
");
                    DeviceConfiguration config = new DeviceConfiguration
                    {
                        ColorFormat = ImageFormat.ColorBGRA32,
                        ColorResolution = ColorResolution.R1080p,
                        DepthMode = DepthMode.PassiveIR,
                        CameraFPS = FPS.FPS15,
                        SynchronizedImagesOnly = true,
                        DepthDelayOffColor = System.TimeSpan.FromSeconds(-1),
                        WiredSyncMode = WiredSyncMode.Master,
                        SuboridinateDelayOffMaster = System.TimeSpan.FromMilliseconds(500),
                        DisableStreamingIndicator = true
                    };

                    device.StartCameras(config);

                    Assert.AreEqual(2, count.Calls("k4a_device_start_cameras"));

                    device.Dispose();

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        device.StartCameras(config);
                    });
                }
            }
        }

        [Test]
        public void DeviceStartCamerasFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_start_cameras(
    k4a_device_t device_handle,
    const k4a_device_configuration_t * config
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    STUB_ASSERT(config != NULL);

    return K4A_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    DeviceConfiguration config = new DeviceConfiguration
                    {
                        ColorFormat = ImageFormat.ColorBGRA32,
                        ColorResolution = ColorResolution.R1080p,
                        DepthMode = DepthMode.PassiveIR,
                        CameraFPS = FPS.FPS15,
                        SynchronizedImagesOnly = true,
                        DepthDelayOffColor = System.TimeSpan.FromSeconds(-1),
                        WiredSyncMode = WiredSyncMode.Master,
                        SuboridinateDelayOffMaster = System.TimeSpan.FromMilliseconds(500),
                        DisableStreamingIndicator = true
                    };

                    Assert.Throws(typeof(AzureKinectStartCamerasException), () =>
                    {
                        device.StartCameras(config);
                    });

                    Assert.AreEqual(1, count.Calls("k4a_device_start_cameras"));
                    Assert.AreEqual(0, count.Calls("k4a_device_stop_cameras"));
                }
            }
        }

        [Test]
        public void DeviceStopCameras()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
void k4a_device_stop_cameras(
    k4a_device_t device_handle
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    device.StopCameras();
                    Assert.AreEqual(1, count.Calls("k4a_device_stop_cameras"));

                    device.Dispose();

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        device.StopCameras();
                    });
                }
            }
        }

        [Test]
        public void DeviceStartImu()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_start_imu(
    k4a_device_t device_handle
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    
    return K4A_RESULT_SUCCEEDED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    device.StartImu();
                    Assert.AreEqual(1, count.Calls("k4a_device_start_imu"));

                    device.Dispose();

                    Assert.Throws(typeof(ObjectDisposedException), () =>
                    {
                        device.StartImu();
                    });
                }
            }
        }

        [Test]
        public void DeviceStartImuFailure()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
k4a_result_t k4a_device_start_imu(
    k4a_device_t device_handle
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
    
    return K4A_RESULT_FAILED;
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    Assert.Throws(typeof(AzureKinectStartImuException), () =>
                    {
                        device.StartImu();
                    });

                    Assert.AreEqual(1, count.Calls("k4a_device_start_imu"));
                }
            }
        }

        [Test]
        public void DeviceStopImu()
        {
            SetOpenCloseImplementation();

            NativeK4a.SetImplementation(@"
void k4a_device_stop_imu(
    k4a_device_t device_handle
)
{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
}
");
            {
                CallCount count = NativeK4a.CountCalls();
                using (Device device = Device.Open(0))
                {
                    device.StopImu();
                    Assert.AreEqual(1, count.Calls("k4a_device_stop_imu"));

                    device.Dispose();

                    Assert.Throws(typeof(System.ObjectDisposedException), () =>
                    {
                        device.StopImu();
                    });
                }
            }
        }
    }
}