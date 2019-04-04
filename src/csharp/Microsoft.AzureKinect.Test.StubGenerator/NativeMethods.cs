using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    static class NativeMethods
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate void RaiseError([MarshalAs(UnmanagedType.LPStr)]string szFile, int line, [MarshalAs(UnmanagedType.LPStr)]string expression);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        public delegate void Stub_SetErrorFunction(RaiseError errorFunction);

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Ansi)]
        public static extern IntPtr LoadLibrary([MarshalAs(UnmanagedType.LPStr)]string lpFileName);

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Ansi)]
        public static extern bool FreeLibrary(IntPtr hModule);

        [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("k4a", CharSet = CharSet.Ansi)]
        public static extern int Stub_RegisterRedirect(string functionName, IntPtr implementation);

        [DllImport("k4a", CharSet = CharSet.Ansi)]
        public static extern int Stub_GetCallCount(string functionName);

        

    }
}
