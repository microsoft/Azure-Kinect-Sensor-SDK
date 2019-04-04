using NUnit.Framework;
using Microsoft.AzureKinect;
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
            Assert.AreEqual(depthMode, cal.depth_mode);
            Assert.AreEqual(colorResolution, cal.color_resolution);
            Assert.AreEqual(depthWidth, cal.depth_camera_calibration.resolution_width);
            Assert.AreEqual(depthHeight, cal.depth_camera_calibration.resolution_height);
            Assert.AreEqual(colorWidth, cal.color_camera_calibration.resolution_width);
            Assert.AreEqual(colorHeight, cal.color_camera_calibration.resolution_height);
            Assert.IsTrue(cal.depth_camera_calibration.intrinsics.type == Calibration.ModelType.Rational6KT ||
                cal.depth_camera_calibration.intrinsics.type == Calibration.ModelType.BrownConrady);
            Assert.IsTrue(cal.color_camera_calibration.intrinsics.type == Calibration.ModelType.Rational6KT ||
                cal.color_camera_calibration.intrinsics.type == Calibration.ModelType.BrownConrady);
        }

        [Test]
        public void GetFromRaw()
        {
            using (Device device = Device.Open(0))
            {
                byte[] raw = device.GetRawCalibration();

                Calibration cal = Calibration.GetFromRaw(raw, DepthMode.WFOV_2x2Binned, ColorResolution.r1080p);

                // Sanity check a few of the outputs for well known fields

                ValidateCalibration(cal, DepthMode.WFOV_2x2Binned, ColorResolution.r1080p,
                    512, 512,
                    1920, 1080);

                cal = Calibration.GetFromRaw(raw, DepthMode.WFOV_Unbinned, ColorResolution.r720p);

                ValidateCalibration(cal, DepthMode.WFOV_Unbinned, ColorResolution.r720p,
                    1024, 1024,
                    1280, 720);
            }
        }

        [Test]
        public void Transform_2d_to_2d()
        {
            using (Device device = Device.Open(0))
            {
                byte[] raw = device.GetRawCalibration();

                Calibration cal = Calibration.GetFromRaw(raw, DepthMode.WFOV_2x2Binned, ColorResolution.r1080p);

                Float2 source = new Float2(0, 0);
                Float2 result = cal.Transform_2d_to_2d(source, 1.0f, Calibration.DeviceType.Color, Calibration.DeviceType.Depth);

            }
        }
    }
}
