using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect.Native
{

    /// <summary>
    /// Attribute indicating the native equivilant.
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
