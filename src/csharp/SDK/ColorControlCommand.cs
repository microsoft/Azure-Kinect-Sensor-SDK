// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_color_control_command_t")]
    public enum ColorControlCommand
    {
        [Native.NativeReference("K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE")]
        ExposureTimeAbsolute,
        [Native.NativeReference("K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY")]
        AutoExposurePriority,
        Brightness,
        Contrast,
        Saturation,
        Sharpness,
        Whitebalance,
        BacklightCompensation,
        Gain,
        PowerlineFrequency
    }
}
