// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System.Numerics;
using Microsoft.Azure.Kinect.Sensor;
using NUnit.Framework;

namespace WrapperTests
{
    public class CalibrationTests
    {


        void ValidateCalibration(Calibration cal,
            DepthMode depthMode,
            ColorResolution colorResolution,
            int depthWidth, int depthHeight,
            int colorWidth, int colorHeight)
        {
            Assert.AreEqual(depthMode, cal.DepthMode);
            Assert.AreEqual(colorResolution, cal.ColorResolution);
            Assert.AreEqual(depthWidth, cal.DepthCameraCalibration.ResolutionWidth);
            Assert.AreEqual(depthHeight, cal.DepthCameraCalibration.ResolutionHeight);
            Assert.AreEqual(colorWidth, cal.ColorCameraCalibration.ResolutionWidth);
            Assert.AreEqual(colorHeight, cal.ColorCameraCalibration.ResolutionHeight);
            Assert.IsTrue(cal.DepthCameraCalibration.Intrinsics.Type == CalibrationModelType.Rational6KT ||
                cal.DepthCameraCalibration.Intrinsics.Type == CalibrationModelType.BrownConrady);
            Assert.IsTrue(cal.ColorCameraCalibration.Intrinsics.Type == CalibrationModelType.Rational6KT ||
                cal.ColorCameraCalibration.Intrinsics.Type == CalibrationModelType.BrownConrady);
        }

        [Test]
        public void GetFromRaw()
        {
            using (Device device = Device.Open(0))
            {
                byte[] raw = device.GetRawCalibration();

                Calibration cal = Calibration.GetFromRaw(raw, DepthMode.WFOV_2x2Binned, ColorResolution.R1080p);

                // Sanity check a few of the outputs for well known fields

                ValidateCalibration(cal, DepthMode.WFOV_2x2Binned, ColorResolution.R1080p,
                    512, 512,
                    1920, 1080);

                cal = Calibration.GetFromRaw(raw, DepthMode.WFOV_Unbinned, ColorResolution.R720p);

                ValidateCalibration(cal, DepthMode.WFOV_Unbinned, ColorResolution.R720p,
                    1024, 1024,
                    1280, 720);
            }
        }

        [Test]
        public void Transform2Dto2D()
        {
            using (Device device = Device.Open(0))
            {
                byte[] raw = device.GetRawCalibration();

                Calibration cal = Calibration.GetFromRaw(raw, DepthMode.WFOV_2x2Binned, ColorResolution.R1080p);

                Vector2 source = new Vector2(0, 0);
                Vector2? result = cal.TransformTo2D(source, 1.0f, CalibrationDeviceType.Color, CalibrationDeviceType.Depth);

            }
        }
    }
}
