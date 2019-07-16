// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Documents;
using Microsoft.Azure.Kinect.Sensor;
using Microsoft.Azure.Kinect.Sensor.WPF;

namespace Microsoft.Azure.Kinect.Sensor.Examples.WPFViewer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private async void Window_Loaded(object sender, RoutedEventArgs e)
        {
            using (Device device = Device.Open(0))
            {
                device.StartCameras(new DeviceConfiguration {
                    ColorFormat = ImageFormat.ColorBGRA32,
                    ColorResolution = ColorResolution.r1440p,
                    DepthMode = DepthMode.WFOV_2x2Binned,
                    SynchronizedImagesOnly = true,
                    CameraFPS = FPS.fps30,
                });

                int colorWidth = device.GetCalibration().color_camera_calibration.resolution_width;
                int colorHeight = device.GetCalibration().color_camera_calibration.resolution_height;

                DateTime start = DateTime.Now;
                int frameCount = 0;

                // Allocate image buffers for us to manipulate
                using (Image transformedDepth = new Image(ImageFormat.Depth16, colorWidth, colorHeight))
                using (Transformation transform = device.GetCalibration().CreateTransformation())
                {
                    while (true)
                    {
                        // Wait for a capture on a thread pool thread
                        using (Capture capture = await Task.Run(() => { return device.GetCapture(); }).ConfigureAwait(true))
                        {
                            // Update the color image preview
                            this.colorImageViewPane.Source = capture.Color.CreateBitmapSource();

                            // Compute the depth preview on a thread pool thread
                            await Task.Run(() =>
                            {
                                // Transform the depth image
                                transform.DepthImageToColorCamera(capture, transformedDepth);

                                Span<ushort> depthBuffer = MemoryMarshal.Cast<byte, ushort>(transformedDepth.Memory.Span);
                                Span<BGRA> colorBuffer = MemoryMarshal.Cast<byte, BGRA>(capture.Color.Memory.Span);

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
                            }).ConfigureAwait(true);

                            this.depthImageViewPane.Source = capture.Color.CreateBitmapSource();
                            frameCount++;

                            TimeSpan timeSpan = DateTime.Now - start;
                            if (timeSpan > TimeSpan.FromSeconds(2))
                            {
                                double framesPerSecond = (double)frameCount / timeSpan.TotalSeconds;

                                this.fps.Content = $"{framesPerSecond:F2} FPS";

                                frameCount = 0;
                                start = DateTime.Now;
                            }
                        }
                    }
                }
            }
        }
    }
}
