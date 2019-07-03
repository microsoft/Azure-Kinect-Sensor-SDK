// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor
{
    [Native.NativeReference("k4a_depth_mode_t")]
    public enum DepthMode
    {
#pragma warning disable CA1707 // Identifiers should not contain underscores
        Off = 0,
        NFOV_2x2Binned,
        NFOV_Unbinned,
        WFOV_2x2Binned,
        WFOV_Unbinned,
        PassiveIR
#pragma warning restore CA1707 // Identifiers should not contain underscores
    }
}
