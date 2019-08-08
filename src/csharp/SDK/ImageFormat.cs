//------------------------------------------------------------------------------
// <copyright file="ImageFormat.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Image format.
    /// </summary>
    [Native.NativeReference("k4a_image_format_t")]
    public enum ImageFormat
    {
        /// <summary>
        /// Color image type MJPG.
        /// </summary>
        /// <remarks>
        /// The buffer for each image is encoded as a JPEG and can be decoded by a JPEG decoder.
        ///
        /// Because the image is compressed, the Stride property of the Image is not applicable.
        ///
        /// Each MJPG encoded image in a stream may be of differing size depending on the compression efficiency.
        /// </remarks>
        ColorMJPG = 0,

        /// <summary>
        /// Color image type NV12.
        /// </summary>
        /// <remarks>
        /// NV12 images separate the luminance and chroma data such that all the luminance is at the beginning
        /// of the buffer, and the chroma lines follow immediately after.
        ///
        /// Stride indicates the length of each line in bytes and should be used to determine the start location
        /// of each line of the image in memory. Chroma has half as many lines of height and half the width in
        /// pixels of the luminance. Each chroma line has the same width in bytes as a luminance line.
        /// </remarks>
        ColorNV12,

        /// <summary>
        /// Color image type YUY2.
        /// </summary>
        /// <remarks>
        /// YUY2 stores chroma and luminance data in interleaved pixels.
        ///
        /// Stride indicates the length of each line in bytes and should be used to determine the start location
        /// of each line of the image in memory.
        /// </remarks>
        ColorYUY2,

        /// <summary>
        /// Color image type BGRA32.
        /// </summary>
        /// <remarks>
        /// Each pixel of BGRA32 data is four bytes. The first three bytes represent Blue, Green, and Red data.
        /// The fourth byte is the alpha channel and is unused in the Azure Kinect APIs.
        ///
        /// Stride indicates the length of each line in bytes and should be used to determine the start location
        /// of each line of the image in memory.
        ///
        /// The Azure Kinect device does not natively capture in this format. Requesting images of this format
        /// requires additional computation in the API.
        /// </remarks>
        ColorBGRA32,

        /// <summary>
        /// Depth image type DEPTH16.
        /// </summary>
        /// <remarks>
        /// Each pixel of DEPTH16 data is two bytes of little-endian unsigned depth data. The unit of the data
        /// is in millimeters from the origin of the camera.
        ///
        /// Stride indicates the length of each line in bytes and should be used to determine the start location
        /// of each line of the image in memory.
        /// </remarks>
        Depth16,

        /// <summary>
        /// Image type IR16.
        /// </summary>
        /// <remarks>
        /// Each pixel of IR16 data is two bytes of little-endian unsigned depth data. The value of the data
        /// represents brightness.
        ///
        /// This format represents infrared light and is captured by the depth camera. Stride indicates the
        /// length of each line in bytes and should be used to determine the start location of each line of
        /// the image in memory.
        /// </remarks>
        IR16,

        /// <summary>
        /// Single channel image type CUSTOM8.
        /// </summary>
        /// <remarks>
        /// Each pixel of CUSTOM8 is a single channel one byte of unsigned data.
        ///
        /// Stride indicates the length of each line in bytes and should be used to determine the start
        /// location of each line of the image in memory.
        /// </remarks>
        Custom8,

        /// <summary>
        /// Single channel image type CUSTOM16.
        /// </summary>
        /// <remarks>
        /// Each pixel of CUSTOM16 is a single channel two bytes of little-endian unsigned data.
        ///
        /// Stride indicates the length of each line in bytes and should be used to determine the start
        /// location of each line of the image in memory.
        /// </remarks>
        Custom16,

        /// <summary>
        /// Custom image format.
        /// </summary>
        /// <remarks>
        /// Used in conjunction with user created images or images packing non-standard data.
        ///
        /// See the originator of the custom formatted image for information on how to interpret the data.
        /// </remarks>
        Custom,
    }
}
