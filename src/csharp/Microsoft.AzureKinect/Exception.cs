using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{

    [Serializable]
    public class Exception : System.Exception
    {
        public Exception(string message) : base(message) { }

        internal static void ThrowIfNotSuccess(NativeMethods.k4a_result_t result)
        {
            if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
            {
                throw new Exception($"result = {result}");
            }
        }

        internal static void ThrowIfNotSuccess(NativeMethods.k4a_wait_result_t result)
        {
            if (result != NativeMethods.k4a_wait_result_t.K4A_WAIT_RESULT_SUCCEEDED)
            {
                throw new Exception($"result = {result}");
            }
        }

        internal static void ThrowIfNotSuccess(NativeMethods.k4a_buffer_result_t result)
        {
            if (result != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_SUCCEEDED)
            {
                throw new Exception($"result = {result}");
            }
        }
    }
}
