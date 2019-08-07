//------------------------------------------------------------------------------
// <copyright file="PlaybackSeekOrigin.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    /// <summary>
    /// Playback seeking positions.
    /// </summary>
    [Native.NativeReference("k4a_playback_seek_origin_t")]
    public enum PlaybackSeekOrigin
    {
        /// <summary>
        /// Seek relative to the beginning of a recording.
        /// </summary>
        Begin,

        /// <summary>
        /// Seek relative to the end of a recording.
        /// </summary>
        End,

        /// <summary>
        /// Seek to an absolute device time-stamp.
        /// </summary>
        DeviceTime,
    }
}
