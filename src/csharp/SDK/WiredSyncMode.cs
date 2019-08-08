//------------------------------------------------------------------------------
// <copyright file="WiredSyncMode.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Synchronization mode when connecting two or more devices together.
    /// </summary>
    [Native.NativeReference("k4a_wired_sync_mode_t")]
    public enum WiredSyncMode
    {
        /// <summary>
        /// Neither 'Sync In' or 'Sync Out' connections are used.
        /// </summary>
        Standalone,

        /// <summary>
        /// The 'Sync Out' jack is enabled and synchronization data it driven out the connected wire.
        /// </summary>
        /// <remarks>
        /// While in master mode the color camera must be enabled as part of the multi device sync
        /// signaling logic. Even if the color image is not needed, the color camera must be running.
        /// </remarks>
        Master,

        /// <summary>
        /// The 'Sync In' jack is used for synchronization and 'Sync Out' is driven for the next device in the chain.
        /// </summary>
        /// <remarks>
        /// 'Sync Out' is a mirror of 'Sync In' for this mode.
        /// </remarks>
        Subordinate,
    }
}
