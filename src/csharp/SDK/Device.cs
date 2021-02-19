//------------------------------------------------------------------------------
// <copyright file="Device.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Text;
using static Microsoft.Azure.Kinect.Sensor.NativeMethods;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents an Azure Kinect device.
    /// </summary>
    public class Device : IDisposable
    {
        // Cache these values so we don't need to re-marshal them for each
        // access since they are immutable.
        private string serialNum = null;
        private HardwareVersion version = null;

        // The native handle to the device.
        private NativeMethods.k4a_device_t handle;

        // To detect redundant calls to Dispose
        private bool disposedValue = false;

        private Device(NativeMethods.k4a_device_t handle)
        {
            // Hook the native allocator and register this object.
            // .Dispose() will be called on this object when the allocator is shut down.
            Allocator.Singleton.RegisterForDisposal(this);

            this.handle = handle;
        }

        /// <summary>
        /// Gets the devices serial number.
        /// </summary>
        public string SerialNum
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Device));
                    }

                    if (this.serialNum != null)
                    {
                        return this.serialNum;
                    }
                    else
                    {
                        // Determine the required string size
                        UIntPtr size = new UIntPtr(0);
                        if (NativeMethods.k4a_device_get_serialnum(this.handle, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                        {
                            throw new InvalidOperationException($"Unexpected internal state calling {nameof(NativeMethods.k4a_device_get_serialnum)}");
                        }

                        // Allocate a string buffer
                        StringBuilder serialno = new StringBuilder((int)size.ToUInt32());

                        // Get the serial number
                        AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_get_serialnum(this.handle, serialno, ref size));

                        this.serialNum = serialno.ToString();

                        return this.serialNum;
                    }
                }
            }
        }

        /// <summary>
        /// Gets the depth mode the device is currently set to.
        /// </summary>
        public int CurrentDepthModeId { get; private set; } = 0; // 0 = Off

        /// <summary>
        /// Gets the color resolution the device is currently set to.
        /// </summary>
        public int CurrentColorModeId { get; private set; } = 0; // 0 = Off

        /// <summary>
        /// Gets a value indicating whether gets the Sync In jack is connected.
        /// </summary>
        public bool SyncInJackConnected
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Device));
                    }

                    bool sync_in = default;
                    bool sync_out = default;
                    AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_get_sync_jack(
                        this.handle,
                        out sync_in,
                        out sync_out));

                    return sync_in;
                }
            }
        }

        /// <summary>
        /// Gets a value indicating whether gets the Sync Out jack is connected.
        /// </summary>
        public bool SyncOutJackConnected
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Device));
                    }

                    bool sync_in = default;
                    bool sync_out = default;
                    AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_get_sync_jack(
                        this.handle,
                        out sync_in,
                        out sync_out));

                    return sync_out;
                }
            }
        }

        /// <summary>
        /// Gets the hardware version of the device.
        /// </summary>
        public HardwareVersion Version
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Device));
                    }

                    if (this.version != null)
                    {
                        return this.version;
                    }

                    NativeMethods.k4a_hardware_version_t nativeVersion = default;
                    AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_get_version(
                        this.handle,
                        out nativeVersion));

                    this.version = nativeVersion.ToHardwareVersion();
                    return this.version;
                }
            }
        }

        /// <summary>
        /// Gets the number of currently connected devices.
        /// </summary>
        /// <returns>The number of connected devices.</returns>
        public static int GetInstalledCount()
        {
            return (int)NativeMethods.k4a_device_get_installed_count();
        }

        /// <summary>
        /// Opens an Azure Kinect device.
        /// </summary>
        /// <param name="index">Index of the device to open if there are multiple connected.</param>
        /// <returns>A Device object representing that device.</returns>
        /// <remarks>The device will remain opened for exclusive access until the Device object is disposed.</remarks>
        public static Device Open(int index = 0)
        {
            NativeMethods.k4a_device_t handle = default;
            AzureKinectOpenDeviceException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_open((uint)index, out handle));
            return new Device(handle);
        }

        /// <summary>
        /// Gets the calibration of the device.
        /// </summary>
        /// <param name="depthModeId">Depth mode id for the calibration.</param>
        /// <param name="colorModeId">Color modes id for the calibration.</param>
        /// <returns>Calibration object.</returns>
        public Calibration GetCalibration(int depthModeId, int colorModeId)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                Calibration calibration = default;
                AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_get_calibration(this.handle, (uint) depthModeId, (uint) colorModeId, out calibration));
                return calibration;
            }
        }

        /// <summary>
        /// Gets the calibration of the device for the current operating mode.
        /// </summary>
        /// <returns>Calibration object.</returns>
        public Calibration GetCalibration()
        {
            if (this.CurrentColorModeId == 0 && this.CurrentDepthModeId == 0)
            {
                throw new AzureKinectException("Cameras not started");
            }

            return this.GetCalibration(this.CurrentDepthModeId, this.CurrentColorModeId);
        }

        /// <summary>
        /// Gets the device raw calibration data.
        /// </summary>
        /// <returns>The raw data can be stored off-line for future use.</returns>
        public byte[] GetRawCalibration()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                // Determine the required calibration size
                UIntPtr size = new UIntPtr(0);
                if (NativeMethods.k4a_device_get_raw_calibration(this.handle, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                {
                    throw new AzureKinectException($"Unexpected result calling {nameof(NativeMethods.k4a_device_get_raw_calibration)}");
                }

                // Allocate a string buffer
                byte[] raw = new byte[size.ToUInt32()];

                // Get the raw calibration
                AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_get_raw_calibration(this.handle, raw, ref size));

                return raw;
            }
        }

        /// <summary>
        /// Reads a sensor capture.
        /// </summary>
        /// <param name="timeout">Time to wait for a capture.</param>
        /// <returns>A Capture object holding image data.</returns>
        /// <remarks>
        /// Gets the next capture in the streamed sequence of captures from the camera.
        /// If a new capture is not currently available, this function will block until the timeout is reached.
        /// The SDK will buffer at least two captures worth of data before dropping the oldest capture.
        /// Callers needing to capture all data need to ensure they read the data as fast as the data is being produced on average.
        /// </remarks>
        public Capture GetCapture(TimeSpan timeout)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                using (LoggingTracer tracer = new LoggingTracer())
                {
                    NativeMethods.k4a_wait_result_t result = NativeMethods.k4a_device_get_capture(this.handle, out NativeMethods.k4a_capture_t capture, (int)timeout.TotalMilliseconds);

                    if (result == NativeMethods.k4a_wait_result_t.K4A_WAIT_RESULT_TIMEOUT)
                    {
                        throw new TimeoutException("Timed out waiting for capture");
                    }

                    AzureKinectException.ThrowIfNotSuccess(tracer, result);

                    if (capture.IsInvalid)
                    {
                        throw new AzureKinectException("k4a_device_get_capture did not return a valid capture handle");
                    }

                    return new Capture(capture);
                }
            }
        }

        /// <summary>
        /// Reads a sensor capture.
        /// </summary>
        /// <returns>A Capture object holding image data.</returns>
        /// <remarks>
        /// Gets the next capture in the streamed sequence of captures from the camera.
        /// If a new capture is not currently available, this function will block until the timeout is reached.
        /// The SDK will buffer at least two captures worth of data before dropping the oldest capture.
        /// Callers needing to capture all data need to ensure they read the data as fast as the data is being produced on average.
        /// </remarks>
        public Capture GetCapture()
        {
            return this.GetCapture(TimeSpan.FromMilliseconds(-1));
        }

        /// <summary>
        /// Reads an IMU sample from the device.
        /// </summary>
        /// <param name="timeout">Time to wait for an IMU sample.</param>
        /// <returns>The next unread IMU sample from the device.</returns>
        /// <remarks>Gets the next sample in the streamed sequence of IMU samples from the device.
        /// If a new sample is not currently available, this function will block until the timeout is reached.
        /// The API will buffer at least two camera capture intervals worth of samples before dropping the oldest sample. Callers needing to capture all data need to ensure they read the data as fast as the data is being produced on average.
        /// </remarks>
        public ImuSample GetImuSample(TimeSpan timeout)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                using (LoggingTracer tracer = new LoggingTracer())
                {
                    NativeMethods.k4a_imu_sample_t sample = new NativeMethods.k4a_imu_sample_t();
                    NativeMethods.k4a_wait_result_t result = NativeMethods.k4a_device_get_imu_sample(this.handle, sample, (int)timeout.TotalMilliseconds);

                    if (result == NativeMethods.k4a_wait_result_t.K4A_WAIT_RESULT_TIMEOUT)
                    {
                        throw new TimeoutException("Timed out waiting for IMU sample");
                    }

                    AzureKinectException.ThrowIfNotSuccess(tracer, result);

                    return sample.ToImuSample();
                }
            }
        }

        /// <summary>
        /// Reads an IMU sample from the device.
        /// </summary>
        /// <returns>The next unread IMU sample from the device.</returns>
        /// <remarks>Gets the next sample in the streamed sequence of IMU samples from the device.
        /// If a new sample is not currently available, this function will block until one is available.
        /// The API will buffer at least two camera capture intervals worth of samples before dropping the oldest sample. Callers needing to capture all data need to ensure they read the data as fast as the data is being produced on average.
        /// </remarks>
        public ImuSample GetImuSample()
        {
            return this.GetImuSample(TimeSpan.FromMilliseconds(-1));
        }

        /// <summary>
        /// Get the Azure Kinect color sensor control value.
        /// </summary>
        /// <param name="command">Color sensor control command.</param>
        /// <returns>The value of the color control option.</returns>
        public int GetColorControl(ColorControlCommand command)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                return this.GetColorControl(command, out ColorControlMode mode);
            }
        }

        /// <summary>
        /// Get the Azure Kinect color sensor control value.
        /// </summary>
        /// <param name="command">Color sensor control command.</param>
        /// <param name="mode">The mode of the color control option.</param>
        /// <returns>The value of the color control option.</returns>
        public int GetColorControl(ColorControlCommand command, out ColorControlMode mode)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                using (LoggingTracer tracer = new LoggingTracer())
                {
                    AzureKinectException.ThrowIfNotSuccess(tracer, NativeMethods.k4a_device_get_color_control(this.handle, command, out mode, out int value));
                    return value;
                }
            }
        }

        /// <summary>
        /// Sets the Azure Kinect color sensor control value.
        /// </summary>
        /// <param name="command">Color sensor control command.</param>
        /// <param name="mode">The mode of the color control option.</param>
        /// <param name="value">The value of the color control option.</param>
        public void SetColorControl(ColorControlCommand command, ColorControlMode mode, int value)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                AzureKinectException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_set_color_control(this.handle, command, mode, value));
            }
        }

        /// <summary>
        /// Starts color and depth camera capture.
        /// </summary>
        /// <param name="configuration">The configuration we want to run the device in.</param>
        public void StartCameras(DeviceConfiguration configuration)
        {
            if (configuration == null)
            {
                throw new ArgumentNullException(nameof(configuration));
            }

            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                NativeMethods.k4a_device_configuration_t nativeConfig = configuration.GetNativeConfiguration();
                AzureKinectStartCamerasException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_start_cameras(this.handle, ref nativeConfig));

                this.CurrentDepthModeId = configuration.DepthModeId;
                this.CurrentColorModeId = configuration.ColorModeId;
            }
        }

        /// <summary>
        /// Stops the color and depth camera capture.
        /// </summary>
        public void StopCameras()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                NativeMethods.k4a_device_stop_cameras(this.handle);

                this.CurrentDepthModeId = 0;
                this.CurrentColorModeId = 0;
            }
        }

        /// <summary>
        /// Starts the IMU sample stream.
        /// </summary>
        public void StartImu()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                AzureKinectStartImuException.ThrowIfNotSuccess(() => NativeMethods.k4a_device_start_imu(this.handle));
            }
        }

        /// <summary>
        /// Stops the IMU sample stream.
        /// </summary>
        public void StopImu()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                NativeMethods.k4a_device_stop_imu(this.handle);
            }
        }

        /// <summary>
        /// Get the Device Info.
        /// </summary>
        /// <returns>The Device Info.</returns>
        public DeviceInfo GetInfo()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                DeviceInfo deviceInfo = new DeviceInfo();
                k4a_device_info_t device_info = deviceInfo.GetNativeConfiguration();
                AzureKinectException.ThrowIfNotSuccess(() => k4a_device_get_info(this.handle, out device_info));
                deviceInfo.SetUsingNativeConfiguration(device_info);
                return deviceInfo;
            }
        }

        /// <summary>
        /// Lists the ColorMode Info.
        /// </summary>
        /// <returns>The ColorMode Info.</returns>
        public List<ColorModeInfo> GetColorModes()
        {
            List<ColorModeInfo> colorModes = new List<ColorModeInfo>();
            uint colorModeInfoCount = 0;

            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                AzureKinectException.ThrowIfNotSuccess(() => k4a_device_get_color_mode_count(this.handle, out colorModeInfoCount));
            }

            for (int i = 0; i < colorModeInfoCount; i++)
            {
                ColorModeInfo colorModeInfo = this.GetColorMode(i);
                if (colorModeInfo != null)
                {
                    colorModes.Add(colorModeInfo);
                }
            }


            return colorModes;
        }

        /// <summary>
        /// Get the ColorMode Info.
        /// </summary>
        /// <param name="colorModeId">colorModeid</param>
        /// <returns>The ColorMode Info.</returns>
        public ColorModeInfo GetColorMode(int colorModeId)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }
                ColorModeInfo colorModeInfo = new ColorModeInfo();
                k4a_color_mode_info_t color_mode_info = colorModeInfo.GetNativeConfiguration();
                AzureKinectException.ThrowIfNotSuccess(() => k4a_device_get_color_mode(this.handle, (uint)colorModeId, out color_mode_info));
                colorModeInfo.SetUsingNativeConfiguration(color_mode_info);
                return colorModeInfo;
            }
        }

        /// <summary>
        /// Lists the DepthMode Info.
        /// </summary>
        /// <returns>The DepthModes.</returns>
        public List<DepthModeInfo> GetDepthModes()
        {
            List<DepthModeInfo> depthModes = new List<DepthModeInfo>();
            uint depthModeInfoCount = 0;

            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                AzureKinectException.ThrowIfNotSuccess(() => k4a_device_get_depth_mode_count(this.handle, out depthModeInfoCount));
            }

            for (int i = 0; i < depthModeInfoCount; i++)
            {
                DepthModeInfo depthModeInfo = this.GetDepthMode(i);
                if (depthModeInfo != null)
                {
                    depthModes.Add(depthModeInfo);
                }
            }

            return depthModes;
        }

        /// <summary>
        /// Get the DepthMode Info.
        /// </summary>
        /// <param name="depthModeId">depthModeId</param>
        /// <returns>The DepthMode Info.</returns>
        public DepthModeInfo GetDepthMode(int depthModeId)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                DepthModeInfo depthModeInfo = new DepthModeInfo();
                k4a_depth_mode_info_t depth_mode_info = depthModeInfo.GetNativeConfiguration();
                AzureKinectException.ThrowIfNotSuccess(() => k4a_device_get_depth_mode(this.handle, (uint)depthModeId, out depth_mode_info));
                depthModeInfo.SetUsingNativeConfiguration(depth_mode_info);
                return depthModeInfo;
            }
        }

        /// <summary>
        /// Lists the FPSMode Info.
        /// </summary>
        /// <returns>The FPSModes.</returns>
        public List<FPSModeInfo> GetFPSModes()
        {
            List<FPSModeInfo> fpsModes = new List<FPSModeInfo>();
            uint fpsModeInfoCount = 0;

            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                AzureKinectException.ThrowIfNotSuccess(() => k4a_device_get_fps_mode_count(this.handle, out fpsModeInfoCount));
            }

            for (int i = 0; i < fpsModeInfoCount; i++)
            {
                FPSModeInfo fpsModeInfo = this.GetFPSMode(i);
                if (fpsModeInfo != null)
                {
                    fpsModes.Add(fpsModeInfo);
                }
            }

            return fpsModes;
        }

        /// <summary>
        /// Get the FPShMode Info.
        /// </summary>
        /// <param name="fpsModeId">fpsModeId</param>
        /// <returns>The FPSMode Info.</returns>
        public FPSModeInfo GetFPSMode(int fpsModeId)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Device));
                }

                FPSModeInfo fpsModeInfo = new FPSModeInfo();
                k4a_fps_mode_info_t fps_mode_info = fpsModeInfo.GetNativeConfiguration();
                AzureKinectException.ThrowIfNotSuccess(() => k4a_device_get_fps_mode(this.handle, (uint)fpsModeId, out fps_mode_info));
                fpsModeInfo.SetUsingNativeConfiguration(fps_mode_info);
                return fpsModeInfo;
            }
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(disposing) below.
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Releases unmanaged and - optionally - managed resources.
        /// </summary>
        /// <param name="disposing"><c>True</c> to release both managed and unmanaged resources; <c>False</c> to release only unmanaged resources.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposedValue)
            {
                if (disposing)
                {
                    Allocator.Singleton.UnregisterForDisposal(this);

                    this.handle.Close();
                    this.handle = null;

                    this.disposedValue = true;
                }
            }
        }
    }
}
