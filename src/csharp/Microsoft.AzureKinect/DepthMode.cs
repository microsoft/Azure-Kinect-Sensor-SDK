using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    [Native.NativeReference("k4a_depth_mode_t")]
    public enum DepthMode
    {
        Off = 0,
        NFOV_2x2Binned,
        NFOV_Unbinned,
        WFOV_2x2Binned,
        WFOV_Unbinned,
        PassiveIR
    }
}
