using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    [Native.NativeReference("k4a_firmware_build_t")]
    public enum FirmwareBuild
    {
        Release = 0,
        Debug
    }
}
