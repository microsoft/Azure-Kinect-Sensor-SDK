//------------------------------------------------------------------------------
// <copyright file="Form1.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Azure.Kinect.Sensor.WinForms;

namespace Microsoft.Azure.Kinect.Sensor.Examples.WinForms
{
    /// <summary>
    /// The main form for the Azure Kinect Sensor SDK WinForms example.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("Maintainability", "CA1501:Avoid excessive inheritance", Justification = "This is the accepted WinForms pattern.")]
    public partial class Form1 : Form
    {
        private bool running = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="Form1"/> class.
        /// </summary>
        public Form1()
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

        private async void Form1_Load(object sender, EventArgs e)
        {
            using (Device device = Device.Open(0))
            {
                // We need to provide mode ids for color, depth and fps for the device configuration passed to the device start cameras method.

                // 1. Get the available modes for device using the get modes methods.
                List<ColorModeInfo> colorModes = device.GetColorModes();
                List<FPSModeInfo> fpsModes = device.GetFPSModes();
                List<DepthModeInfo> depthModes = device.GetDepthModes();

                // 2. For this example, let's find the first color mode that isn't the Off mode.
                ColorModeInfo colorModeInfo = colorModes.Find(c => c.ModeId > 0);

                // 3. For this example, let's find the first depth mode that isn't the Off mode.
                DepthModeInfo depthModeInfo = depthModes.Find(d => d.ModeId > 0);

                // 4. For this example, let's find the fps mode with the highest frames per second.
                fpsModes.Sort((a, b) => b.FPS.CompareTo(a.FPS));
                FPSModeInfo fpsModeInfo = fpsModes.Find(f => f.ModeId != 0);

                // 5. If either a color or depth mode was found that met our example conditions and as long as there was at least one fps mode (there should always be),
                // then the mode ids will be valid parameters for the device configuration passed into the device start camera function.
                device.StartCameras(new DeviceConfiguration
                {
                    ColorFormat = ImageFormat.ColorBGRA32,
                    ColorModeId = colorModeInfo.ModeId,
                    DepthModeId = depthModeInfo.ModeId,
                    SynchronizedImagesOnly = true,
                    FPSModeId = fpsModeInfo.ModeId,
                });

                Stopwatch sw = new Stopwatch();
                int frameCount = 0;
                sw.Start();

                Bitmap depthVisualization = null;
                byte[] rgbValues = null;

                while (this.running)
                {
                    using (Capture capture = await Task.Run(() => device.GetCapture()).ConfigureAwait(true))
                    {
                        Task<Bitmap> colorImageTask = Task.Run(() =>
                        {
                            return capture.Color.CreateBitmap();
                        });

                        Task<Bitmap> depthImageTask = Task.Run(() =>
                        {
                            BitmapData bitmapData;

                            if (depthVisualization == null)
                            {
                                // The bitmap and backing buffer can be allocated just once and re-used to prevent allocations every frame.
                                depthVisualization = new Bitmap(capture.Depth.WidthPixels, capture.Depth.HeightPixels, PixelFormat.Format32bppArgb);
                                bitmapData = depthVisualization.LockBits(new Rectangle(0, 0, depthVisualization.Width, depthVisualization.Height), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                                int bytes = Math.Abs(bitmapData.Stride) * depthVisualization.Height;
                                rgbValues = new byte[bytes];
                            }
                            else
                            {
                                bitmapData = depthVisualization.LockBits(new Rectangle(0, 0, depthVisualization.Width, depthVisualization.Height), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                            }

                            ushort[] depthValues = capture.Depth.GetPixels<ushort>().ToArray();

                            // Loop through and colorize the depth so that we can visual it. Any pixel that has a value of 0 is invalid (too close, too far away, multi-path, etc) and
                            // will be colored Red. Everything else is gray scale based on how far away it is. The values are scaled anything 2 meters or greater is fully white.
                            for (int i = 0; i < depthValues.Length; i++)
                            {
                                ushort depthValue = depthValues[i];

                                if (depthValue == 0)
                                {
                                    // Set the color to Red, 0xFF0000
                                    rgbValues[i * 4] = 0; // Blue
                                    rgbValues[(i * 4) + 1] = 0; // Green
                                    rgbValues[(i * 4) + 2] = 0xFF; // Red
                                }
                                else
                                {
                                    float brightness = depthValue / 2000f;

                                    if (brightness > 1.0f)
                                    {
                                        // Set the color to White, 0xFFFFFF
                                        rgbValues[i * 4] = 0xFF; // Blue
                                        rgbValues[(i * 4) + 1] = 0xFF; // Green
                                        rgbValues[(i * 4) + 2] = 0xFF; // Red
                                    }
                                    else
                                    {
                                        // Set the color to a gray based on how far away the point is.
                                        byte c = (byte)(brightness * 250);
                                        rgbValues[i * 4] = c; // Blue
                                        rgbValues[(i * 4) + 1] = c; // Green
                                        rgbValues[(i * 4) + 2] = c; // Red
                                    }
                                }

                                // Set the Alpha to max since it isn't being used.
                                rgbValues[(i * 4) + 3] = 0xFF;
                            }

                            System.Runtime.InteropServices.Marshal.Copy(rgbValues, 0, bitmapData.Scan0, rgbValues.Length);

                            depthVisualization.UnlockBits(bitmapData);

                            return depthVisualization;
                        });

                        this.pictureBoxColor.Image = await colorImageTask.ConfigureAwait(true);
                        this.pictureBoxDepth.Image = await depthImageTask.ConfigureAwait(true);
                    }

                    this.Update();

                    ++frameCount;

                    if (sw.Elapsed > TimeSpan.FromSeconds(2))
                    {
                        double framesPerSecond = (double)frameCount / sw.Elapsed.TotalSeconds;
                        this.fpsStatusLabel.Text = $"{framesPerSecond:F2} FPS";

                        frameCount = 0;
                        sw.Restart();
                    }
                }
            }
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            Logger.LogMessage -= this.Logger_LogMessage;
        }
    }
}
