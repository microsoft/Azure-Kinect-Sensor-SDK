// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_image_format_t")]
    public enum ImageFormat
    {
        ColorMJPG = 0,
        ColorNV12,
        ColorYUY2,
        ColorBGRA32,
        Depth16,
        IR16,
        Custom
    }
}
