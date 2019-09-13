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
using System.Windows.Media.Imaging;
using Microsoft.Azure.Kinect.Sensor;
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

        private void Logger_LogMessage(LogMessage logMessage)
        {
            if (logMessage.LogLevel < LogLevel.Information)
            {
                Console.WriteLine("{0} [{1}] {2}@{3}: {4}", logMessage.Time, logMessage.LogLevel, logMessage.FileName, logMessage.Line, logMessage.Message);
            }
        }

        private async void Window_Loaded(object sender, RoutedEventArgs e)
        {
            using (Device device = Device.Open(0))
            {
                device.StartCameras(new DeviceConfiguration
                {
                    ColorFormat = ImageFormat.ColorBGRA32,
                    ColorResolution = ColorResolution.R720p,
                    DepthMode = DepthMode.WFOV_2x2Binned,
                    SynchronizedImagesOnly = true,
                    CameraFPS = FPS.FPS30,
                });

                int colorWidth = device.GetCalibration().ColorCameraCalibration.ResolutionWidth;
                int colorHeight = device.GetCalibration().ColorCameraCalibration.ResolutionHeight;

                Stopwatch sw = new Stopwatch();
                int frameCount = 0;
                sw.Start();

                // Allocate image buffers for us to manipulate
                using (Image transformedDepth = new Image(ImageFormat.Depth16, colorWidth, colorHeight))
                using (Image outputColorImage = new Image(ImageFormat.ColorBGRA32, colorWidth, colorHeight))
                using (Transformation transform = device.GetCalibration().CreateTransformation())
                {

                    while (this.running)
                    {
                        if (!Environment.Is64BitProcess)
                        {
                            // In 32-bit the BitmapSource memory runs out quickly and we can hit OutOfMemoryException.
                            // Force garbage collection in each loop iteration to keep memory in check.
                            GC.Collect();
                        }

                        // Wait for a capture on a thread pool thread
                        using (Capture capture = await Task.Run(() => { return device.GetCapture(); }).ConfigureAwait(true))
                        {
                            // Create a BitmapSource for the unmodified color image.
                            // Creating the BitmapSource is slow, so do it asynchronously on another thread
                            Task<BitmapSource> createInputColorBitmapTask = Task.Run(() =>
                            {
                                BitmapSource source = capture.Color.CreateBitmapSource();

                                // Allow the bitmap to move threads
                                source.Freeze();
                                return source;
                            });

                            // Compute the colorized output bitmap on a thread pool thread
                            Task<BitmapSource> createOutputColorBitmapTask = Task.Run(() =>
                            {
                                // Transform the depth image to the perspective of the color camera
                                transform.DepthImageToColorCamera(capture, transformedDepth);

                                // Get Span<T> references to the pixel buffers for fast pixel access.
                                Span<ushort> depthBuffer = transformedDepth.GetPixels<ushort>().Span;
                                Span<BGRA> colorBuffer = capture.Color.GetPixels<BGRA>().Span;
                                Span<BGRA> outputColorBuffer = outputColorImage.GetPixels<BGRA>().Span;

                                // Create an output color image with data from the depth image
                                for (int i = 0; i < colorBuffer.Length; i++)
                                {
                                    // The output image will be the same as the input color image,
                                    // but colorized with Red where there is no depth data, and Green
                                    // where there is depth data at more than 1.5 meters
                                    outputColorBuffer[i] = colorBuffer[i];

                                    if (depthBuffer[i] == 0)
                                    {
                                        outputColorBuffer[i].R = 255;
                                    }
                                    else if (depthBuffer[i] > 1500)
                                    {
                                        outputColorBuffer[i].G = 255;
                                    }
                                }

                                BitmapSource source = outputColorImage.CreateBitmapSource();

                                // Allow the bitmap to move threads
                                source.Freeze();

                                return source;
                            });

                            // Wait for both bitmaps to be ready and assign them.
                            BitmapSource inputColorBitmap = await createInputColorBitmapTask.ConfigureAwait(true);
                            BitmapSource outputColorBitmap = await createOutputColorBitmapTask.ConfigureAwait(true);

                            this.inputColorImageViewPane.Source = inputColorBitmap;
                            this.outputColorImageViewPane.Source = outputColorBitmap;

                            ++frameCount;

                            if (sw.Elapsed > TimeSpan.FromSeconds(2))
                            {
                                double framesPerSecond = (double)frameCount / sw.Elapsed.TotalSeconds;
                                this.fps.Content = $"{framesPerSecond:F2} FPS";

                                frameCount = 0;
                                sw.Restart();
                            }
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
