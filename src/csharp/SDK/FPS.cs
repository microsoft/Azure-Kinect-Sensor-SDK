// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_fps_t")]
#pragma warning disable CA1717 // Only FlagsAttribute enums should have plural names
    public enum FPS
    {
#pragma warning disable CA1712 // Do not prefix enum values with type name
        fps5 = 0,
        fps15,
        fps30
#pragma warning restore CA1712 // Do not prefix enum values with type name
    }

#pragma warning restore CA1717 // Only FlagsAttribute enums should have plural names
}
