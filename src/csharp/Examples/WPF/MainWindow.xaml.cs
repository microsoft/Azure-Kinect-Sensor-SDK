//------------------------------------------------------------------------------
// <copyright file="MainWindow.xaml.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Windows;
using Microsoft.Azure.Kinect.Sensor.WPF;

namespace Microsoft.Azure.Kinect.Sensor.Examples.WPFViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml.
    /// </summary>
    public partial class MainWindow : Window
    {
        private bool running = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainWindow"/> class.
        /// </summary>
        public MainWindow()
        {
            this.InitializeComponent();
            Logger.LogMessage += this.Logger_LogMessage;
        }

        private void Logger_LogMessage(object sender, DebugMessageEventArgs e)
        {
            if (e.LogLevel < LogLevel.Information)
            {
                Console.WriteLine("{0} [{1}] {2}@{3}: {4}", DateTime.Now, e.LogLevel, e.FileName, e.Line, e.Message);
            }
        }

        private async void Window_Loaded(object sender, RoutedEventArgs e)
        {
            using (Device device = Device.Open(0))
            {
                device.StartCameras(new DeviceConfiguration
                {
                    ColorFormat = ImageFormat.ColorBGRA32,
                    ColorResolution = ColorResolution.r1440p,
                    DepthMode = DepthMode.WFOV_2x2Binned,
                    SynchronizedImagesOnly = true,
                });

                int colorWidth = device.GetCalibration().color_camera_calibration.resolution_width;
                int colorHeight = device.GetCalibration().color_camera_calibration.resolution_height;

                // Allocate image buffers for us to manipulate
                using (ArrayImage<BGRA> modifiedColor = new ArrayImage<BGRA>(ImageFormat.ColorBGRA32, colorWidth, colorHeight))
                using (ArrayImage<ushort> transformedDepth = new ArrayImage<ushort>(ImageFormat.Depth16, colorWidth, colorHeight))
                using (Transformation transform = device.GetCalibration().CreateTransformation())
                {
                    Stopwatch sw = new Stopwatch();
                    int frameCount = 0;
                    sw.Start();

                    while (this.running)
                    {
                        // Wait for a capture on a thread pool thread
                        using (Capture capture = await Task.Run(() => { return device.GetCapture(); }))
                        {
                            // Update the color image preview
                            colorImageViewPane.Source = capture.Color.CreateBitmapSource();

                            // Compute the depth preview on a thread pool thread
                            depthImageViewPane.Source = (await Task.Run(() =>
                            {
                                // Transform the depth image
                                transform.DepthImageToColorCamera(capture, transformedDepth);
                                // Copy the color image
                                capture.Color.CopyBytesTo(modifiedColor, 0, 0, (int)modifiedColor.Size);

                                ushort[] depthBuffer = transformedDepth.Buffer;
                                BGRA[] colorBuffer = modifiedColor.Buffer;

                                // Modify the color image with data from the depth image
                                for (int i = 0; i < colorBuffer.Length; i++)
                                {
                                    if (depthBuffer[i] == 0)
                                    {
                                        colorBuffer[i].R = 255;
                                    }
                                    else if (depthBuffer[i] > 1500)
                                    {
                                        colorBuffer[i].G = 255;
                                    }
                                }

                                return modifiedColor;
                            })).CreateBitmapSource();
                        }

                        if (++frameCount >= 30)
                        {
                            Console.WriteLine("{0}ms => {1} FPS", sw.Elapsed.TotalMilliseconds, frameCount / sw.Elapsed.TotalSeconds);
                            sw.Restart();
                            frameCount = 0;
                        }
                    }
                }
            }
        }

        private void Window_Closed(object sender, System.EventArgs e)
        {
            Logger.LogMessage -= this.Logger_LogMessage;
        }
    }
}
