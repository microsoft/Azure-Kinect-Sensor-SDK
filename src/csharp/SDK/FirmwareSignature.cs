// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

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
