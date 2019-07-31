//------------------------------------------------------------------------------
// <copyright file="ImageExtensions.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Buffers;
using System.Drawing;

namespace Microsoft.Azure.Kinect.Sensor.WinForms
{
    /// <summary>
    /// Extends the <see cref="Image"/> providing a way to get a WinForms <see cref="Bitmap"/>
    /// object from the <see cref="Image"/>.
    /// </summary>
    public static class ImageExtensions
    {
        /// <summary>
        /// Creates a WinForms <see cref="Bitmap"/> from the <see cref="Image"/>.
        /// </summary>
        /// <param name="image">The <see cref="Image"/> to convert into a <see cref="Bitmap"/>.</param>
        /// <returns>A <see cref="Bitmap"/> that references the data in <see cref="Image"/>.</returns>
        public static Bitmap CreateBitmap(this Image image)
        {
            if (image is null)
            {
                throw new ArgumentNullException(nameof(image));
            }

            System.Drawing.Imaging.PixelFormat pixelFormat;

            // Take a new reference on the image to ensure that the object
            // cannot be disposed by another thread while we have a copy of its Buffer
            using (Image reference = image.Reference())
            {
                unsafe
                {
                    switch (reference.Format)
                    {
                        case ImageFormat.ColorBGRA32:
                            pixelFormat = System.Drawing.Imaging.PixelFormat.Format32bppArgb;
                            break;
                        case ImageFormat.Depth16:
                        case ImageFormat.IR16:
                            pixelFormat = System.Drawing.Imaging.PixelFormat.Format16bppGrayScale;
                            break;
                        default:
                            throw new AzureKinectException($"Pixel format {reference.Format} cannot be converted to a BitmapSource");
                    }

                    using (MemoryHandle pin = image.Memory.Pin())
                    {
                        return new Bitmap(
                            image.WidthPixels,
                            image.HeightPixels,
                            image.StrideBytes,
                            pixelFormat,
                            (IntPtr)pin.Pointer);
                    }
                }
            }
        }
    }
}
