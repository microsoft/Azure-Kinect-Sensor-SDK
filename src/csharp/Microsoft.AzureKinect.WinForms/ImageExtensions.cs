using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using Microsoft.AzureKinect;

namespace Microsoft.AzureKinect.WinForms
{
    public static class ImageExtensions
    {
        public static Bitmap CreateBitmap(this Image image)
        {
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
                            throw new Exception($"Pixel format {reference.Format} cannot be converted to a BitmapSource");
                    }

                    return new Bitmap(image.WidthPixels,
                                    image.HeightPixels,
                                    image.StrideBytes,
                                    System.Drawing.Imaging.PixelFormat.Format32bppArgb,
                                    (IntPtr)image.Buffer);
                }
            }
        }
    }
}
