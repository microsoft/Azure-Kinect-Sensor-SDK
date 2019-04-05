using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    
    public interface IUnsafeImage : IDisposable
    {
        IntPtr UnsafeBufferPointer { get; }

        long Size { get; }

    }
}
