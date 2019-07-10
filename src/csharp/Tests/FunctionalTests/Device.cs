// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using Microsoft.Azure.Kinect.Sensor;
using NUnit.Framework;

namespace WrapperTests
{
    public class DeviceTests
    {
        int deviceCount = 0;

        [SetUp]
        public void Setup()
        {
            deviceCount = Device.GetInstalledCount();
            TestContext.WriteLine($"Found {deviceCount} devices.");

            Assert.Greater(deviceCount, 0, "Test requires at least one device to be connected");
        }

        [Test]
        public void Enumerate()
        {
            for (int i = 0; i < deviceCount; i++)
            {
                using (Device device = Device.Open(i))
                {
                    string serialno = device.SerialNum;
                    TestContext.WriteLine($"  Device {i}: SerialNum=\"{serialno}\"");
                }
            }
        }

        [Test]
        public void GetRawCalibration()
        {
            using (Device device = Device.Open(0))
            {
                byte[] raw = device.GetRawCalibration();

                bool containsNonZero = false;
                for (int i = 0; i < raw.Length; i++)
                {
                    if (raw[i] != 0)
                    {
                        containsNonZero = true;
                        break;
                    }
                }

                // Validate the buffer is non-zero length and contains data
                Assert.IsTrue(containsNonZero);
                Assert.Greater(raw.Length, 0);

                TestContext.WriteLine($"Raw calibration is {raw.Length} bytes.");
            }
        }

        

        
    }
}