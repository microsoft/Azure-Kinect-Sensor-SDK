using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    [Native.NativeReference("k4a_wired_sync_mode_t")]
    public enum WiredSyncMode
    {
        Standalone,
        Master,
        Subordinate
    }
}
