//------------------------------------------------------------------------------
// <copyright file="LogLevel.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Verbosity levels of debug messaging.
    /// </summary>
    [Native.NativeReference("k4a_log_level_t")]
    public enum LogLevel
    {
        /// <summary>
        /// The most severe level of debug messaging.
        /// </summary>
        [Native.NativeReference("K4A_LOG_LEVEL_CRITICAL")]
        Critical = 0,

        /// <summary>
        /// The second most severe level of debug messaging.
        /// </summary>
        [Native.NativeReference("K4A_LOG_LEVEL_ERROR")]
        Error,

        /// <summary>
        /// The third most severe level of debug messaging.
        /// </summary>
        [Native.NativeReference("K4A_LOG_LEVEL_WARNING")]
        Warning,

        /// <summary>
        /// The second least severe level of debug messaging.
        /// </summary>
        [Native.NativeReference("K4A_LOG_LEVEL_INFO")]
        Information,

        /// <summary>
        /// The lest severe level of debug messaging. This is the most verbose messaging possible.
        /// </summary>
        [Native.NativeReference("K4A_LOG_LEVEL_TRACE")]
        Trace,

        /// <summary>
        /// No logging is performed.
        /// </summary>
        [Native.NativeReference("K4A_LOG_LEVEL_OFF")]
        Off,
    }
}
