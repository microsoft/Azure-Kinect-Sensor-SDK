//------------------------------------------------------------------------------
// <copyright file="Form1.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Diagnostics;
using System.Drawing;
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
                device.StartCameras(new DeviceConfiguration
                {
                    ColorFormat = ImageFormat.ColorBGRA32,
                    ColorResolution = ColorResolution.R1080p,
                    DepthMode = DepthMode.NFOV_2x2Binned,
                    SynchronizedImagesOnly = true,
                });

                Stopwatch sw = new Stopwatch();
                int frameCount = 0;
                sw.Start();

                while (true)
                {
                    using (Capture capture = await Task.Run(() => device.GetCapture()).ConfigureAwait(true))
                    {
                        this.pictureBoxColor.Image = capture.Color.CreateBitmap();

                        this.pictureBoxDepth.Image = await Task.Run(() =>
                        {
                            Bitmap depthVisualization = new Bitmap(capture.Depth.WidthPixels, capture.Depth.HeightPixels, System.Drawing.Imaging.PixelFormat.Format32bppArgb);

                            // TODO: Lock the Bitmap and access the bytes directly?
                            ////BitmapData d = depthVisualization.LockBits(new Rectangle(0, 0, depthVisualization.Width, depthVisualization.Height), System.Drawing.Imaging.ImageLockMode.ReadWrite, System.Drawing.Imaging.PixelFormat.Format32bppArgb);

                            ushort[] depthValues = capture.Depth.GetPixels<ushort>().ToArray();

                            for (int y = 0; y < capture.Depth.HeightPixels; y++)
                            {
                                for (int x = 0; x < capture.Depth.WidthPixels; x++)
                                {
                                    ushort depthValue = depthValues[(y * capture.Depth.WidthPixels) + x];

                                    if (depthValue == 0)
                                    {
                                        depthVisualization.SetPixel(x, y, Color.Red);
                                    }
                                    else if (depthValue == ushort.MaxValue)
                                    {
                                        depthVisualization.SetPixel(x, y, Color.Green);
                                    }
                                    else
                                    {
                                        float brightness = depthValue / 2000f;

                                        if (brightness > 1.0f)
                                        {
                                            depthVisualization.SetPixel(x, y, Color.White);
                                        }
                                        else
                                        {
                                            int c = (int)(brightness * 250);
                                            depthVisualization.SetPixel(x, y, Color.FromArgb(c, c, c));
                                        }
                                    }
                                }
                            }

                            return depthVisualization;
                        }).ConfigureAwait(true);

                        this.Invalidate();
                    }

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
