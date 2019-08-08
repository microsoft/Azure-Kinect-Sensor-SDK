//------------------------------------------------------------------------------
// <copyright file="FPS.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Color and depth sensor frame rate.
    /// </summary>
    /// <remarks>
    /// This enumeration is used to select the desired frame rate to operate the cameras.
    /// The actual frame rate may vary slightly due to dropped data, synchronization variation
    /// between devices, clock accuracy, or if the camera exposure priority mode causes
    /// reduced frame rate.
    /// </remarks>
    [Native.NativeReference("k4a_fps_t")]
#pragma warning disable CA1717 // Only FlagsAttribute enums should have plural names
    public enum FPS
    {
#pragma warning disable CA1712 // Do not prefix enum values with type name

        /// <summary>
        /// 5 Frames per second.
        /// </summary>
        FPS5 = 0,

        /// <summary>
        /// 15 Frames per second.
        /// </summary>
        FPS15,

        /// <summary>
        /// 30 Frames per second.
        /// </summary>
        FPS30,
#pragma warning restore CA1712 // Do not prefix enum values with type name
    }

#pragma warning restore CA1717 // Only FlagsAttribute enums should have plural names
}
