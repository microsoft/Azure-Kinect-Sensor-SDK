using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect
{
    public struct HardwareVersion
    {
        public Version rgb;
        public Version depth;
        public Version audio;
        public Version depth_sensor;
        public FirmwareBuild firmware_build;
        public FirmwareSignature firmware_signature;
    }
}
