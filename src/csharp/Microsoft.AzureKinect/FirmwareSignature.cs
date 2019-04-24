using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    [Native.NativeReference("k4a_firmware_signature_t")]
    public enum FirmwareSignature
    {
        Msft,
        Test,
#pragma warning disable CA1720 // Identifier contains type name
        Unsigned
#pragma warning restore CA1720 // Identifier contains type name
    }
}
