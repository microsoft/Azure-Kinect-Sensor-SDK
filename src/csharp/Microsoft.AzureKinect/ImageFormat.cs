using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
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
