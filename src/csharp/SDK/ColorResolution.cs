// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_color_resolution_t")]
    public enum ColorResolution
    {
        Off = 0,
        r720p,
        r1080p,
        r1440p,
        r1536p,
        r2160p,
        r3072p
    }
}
