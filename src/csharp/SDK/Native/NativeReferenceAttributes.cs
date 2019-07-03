// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;

namespace Microsoft.Azure.Kinect.Sensor.Native
{

    /// <summary>
    /// Attribute indicating the native equivalent.
    /// </summary>
    [System.AttributeUsage(AttributeTargets.All, Inherited = false, AllowMultiple = false)]
    public sealed class NativeReferenceAttribute : Attribute
    {
        readonly string referenceName;
        public NativeReferenceAttribute()
        {
            this.referenceName = null;
        }

        public NativeReferenceAttribute(string referenceName)
        {
            this.referenceName = referenceName;
        }
    }
}
