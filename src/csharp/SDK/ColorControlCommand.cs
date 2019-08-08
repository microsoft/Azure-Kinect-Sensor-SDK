//------------------------------------------------------------------------------
// <copyright file="ColorControlCommand.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Color sensor control commands.
    /// </summary>
    [Native.NativeReference("k4a_color_control_command_t")]
    public enum ColorControlCommand
    {
        /// <summary>
        /// Exposure time setting.
        /// </summary>
        /// <remarks>
        /// May be set to Auto or Manual.
        ///
        /// Exposure time is measured in microseconds.
        /// </remarks>
        [Native.NativeReference("K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE")]
        ExposureTimeAbsolute = 0,

        /* Deprecated prior to release of C# wrapper.
        /// <summary>
        /// Exposure or frame-rate priority setting.
        /// </summary>
        /// <remarks>
        /// May only be set to K4A_COLOR_CONTROL_MODE_MANUAL.
        /// Value of 0 means frame-rate priority. Value of 1 means exposure priority.
        /// Using exposure priority may impact the frame-rate of both the color and depth cameras.
        /// Deprecated starting in 1.1.0. Please discontinue usage, firmware does not support this.
        /// </remarks>
        [Native.NativeReference("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY")]
        AutoExposurePriority = 1,
        */

        /// <summary>
        /// Brightness setting.
        /// </summary>
        /// <remarks>
        /// May only be set to Manual.
        ///
        /// The valid range is 0 to 255. The default value is 128.
        /// </remarks>
        Brightness = 2,

        /// <summary>
        /// Contrast setting.
        /// </summary>
        /// <remarks>
        /// May only be set to Manual.
        /// </remarks>
        Contrast = 3,

        /// <summary>
        /// Saturation setting.
        /// </summary>
        /// <remarks>
        /// May only be set to Manual.</remarks>
        Saturation = 4,

        /// <summary>
        /// Sharpness setting.
        /// </summary>
        /// <remarks>
        /// May only be set to Manual.
        /// </remarks>
        Sharpness = 5,

        /// <summary>
        /// White balance setting.
        /// </summary>
        /// <remarks>
        /// May be set to Auto or Manual.
        ///
        /// The unit is degrees Kelvin. The setting must be set to a value evenly divisible by 10 degrees.
        /// </remarks>
        Whitebalance = 6,

        /// <summary>
        /// Back-light compensation setting.
        /// </summary>
        /// <remarks>
        /// May only be set to Manual.
        ///
        /// Value of 0 means back-light compensation is disabled.
        /// Value of 1 means back-light compensation is enabled.
        /// </remarks>
        BacklightCompensation = 7,

        /// <summary>
        /// Gain setting.
        /// </summary>
        /// <remarks>
        /// May only be set to Manual.
        /// </remarks>
        Gain = 8,

        /// <summary>
        /// Power-line frequency setting.
        /// </summary>
        /// <remarks>
        /// May only be set to Manual.
        ///
        /// Value of 1 sets the power-line compensation to 50 Hz.
        /// Value of 2 sets the power-line compensation to 60 Hz.
        /// </remarks>
        PowerlineFrequency = 9,
    }
}
