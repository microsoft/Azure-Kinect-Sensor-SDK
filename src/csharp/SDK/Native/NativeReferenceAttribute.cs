//------------------------------------------------------------------------------
// <copyright file="NativeReferenceAttribute.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;

namespace Microsoft.Azure.Kinect.Sensor.Native
{
    /// <summary>
    /// Attribute indicating the native equivalent.
    /// </summary>
    [System.AttributeUsage(AttributeTargets.All, Inherited = false, AllowMultiple = false)]
    public sealed class NativeReferenceAttribute : Attribute
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="NativeReferenceAttribute"/> class.
        /// </summary>
        public NativeReferenceAttribute()
        {
            this.ReferenceName = null;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NativeReferenceAttribute"/> class with the specified name.
        /// </summary>
        /// <param name="referenceName">The name of the native function, handle, enumeration, callback, or structure entity that is being referenced.</param>
        public NativeReferenceAttribute(string referenceName)
        {
            this.ReferenceName = referenceName;
        }

        /// <summary>
        /// Gets the name of the native function, handle, enumeration, callback, or structure entity that is being referenced.
        /// </summary>
        public string ReferenceName { get; private set; }
    }
}
