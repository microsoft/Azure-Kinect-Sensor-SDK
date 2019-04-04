using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    [Native.NativeReference("k4a_fps_t")]
    public enum FPS
    {
        fps5 = 0,
        fps15,
        fps30
    }
}
