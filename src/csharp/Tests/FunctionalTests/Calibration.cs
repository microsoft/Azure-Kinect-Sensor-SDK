// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System.Collections.Generic;
using System.Numerics;
using Microsoft.Azure.Kinect.Sensor;
using NUnit.Framework;

namespace WrapperTests
{
    public class CalibrationTests
    {
        void ValidateCalibration(Calibration cal, int depthModeId, int colorModeId, int depthWidth, int depthHeight, int colorWidth, int colorHeight)
        {
            Assert.AreEqual(depthModeId, cal.DepthModeId);
            Assert.AreEqual(colorModeId, cal.ColorModeId);
            Assert.AreEqual(depthWidth, cal.DepthCameraCalibration.ResolutionWidth);
            Assert.AreEqual(depthHeight, cal.DepthCameraCalibration.ResolutionHeight);
            Assert.AreEqual(colorWidth, cal.ColorCameraCalibration.ResolutionWidth);
            Assert.AreEqual(colorHeight, cal.ColorCameraCalibration.ResolutionHeight);
            Assert.IsTrue(cal.DepthCameraCalibration.Intrinsics.Type == CalibrationModelType.Rational6KT || cal.DepthCameraCalibration.Intrinsics.Type == CalibrationModelType.BrownConrady);
            Assert.IsTrue(cal.ColorCameraCalibration.Intrinsics.Type == CalibrationModelType.Rational6KT || cal.ColorCameraCalibration.Intrinsics.Type == CalibrationModelType.BrownConrady);
        }

        [Test]
        public void GetFromRaw()
        {
            using (Device device = Device.Open(0))
            {
                byte[] raw = device.GetRawCalibration();

                List<ColorModeInfo> colorModes = device.GetColorModes();
                List<DepthModeInfo> depthModes = device.GetDepthModes();

                ColorModeInfo colorModeInfo = colorModes.Find(c => c.Height >= 1080);
                DepthModeInfo depthModeInfo = depthModes.Find(d => d.Height >= 512 && d.HorizontalFOV >= 120);

                Calibration cal = Calibration.GetFromRaw(raw, (uint)depthModeInfo.ModeId, (uint)colorModeInfo.ModeId);

                // Sanity check a few of the outputs for well known fields
                this.ValidateCalibration(cal, depthModeInfo.ModeId, colorModeInfo.ModeId, 512, 512, 1920, 1080);

                colorModeInfo = colorModes.Find(c => c.Height >= 720);
                depthModeInfo = depthModes.Find(d => d.Height >= 1024 && d.HorizontalFOV >= 120);
                cal = Calibration.GetFromRaw(raw, (uint)depthModeInfo.ModeId, (uint)colorModeInfo.ModeId);

                this.ValidateCalibration(cal, depthModeInfo.ModeId, colorModeInfo.ModeId, 1024, 1024, 1280, 720);
            }
        }

        [Test]
        public void Transform2Dto2D()
        {
            using (Device device = Device.Open(0))
            {
                byte[] raw = device.GetRawCalibration();


                List<ColorModeInfo> colorModes = device.GetColorModes();
                List<DepthModeInfo> depthModes = device.GetDepthModes();

                ColorModeInfo colorModeInfo = colorModes.Find(c => c.Height >= 1080);
                DepthModeInfo depthModeInfo = depthModes.Find(d => d.Height >= 512 && d.HorizontalFOV >= 120);

                Calibration cal = Calibration.GetFromRaw(raw, (uint)depthModeInfo.ModeId, (uint)colorModeInfo.ModeId);

                Vector2 source = new Vector2(0, 0);
                Vector2? result = cal.TransformTo2D(source, 1.0f, CalibrationDeviceType.Color, CalibrationDeviceType.Depth);

            }
        }
    }
}
