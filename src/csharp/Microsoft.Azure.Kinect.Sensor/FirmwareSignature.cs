using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_firmware_signature_t")]
    public enum FirmwareSignature
    {
        Msft,
        Test,
        NotSigned
    }
}
