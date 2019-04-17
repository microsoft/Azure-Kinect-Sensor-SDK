using System;
using System.IO;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    public static class EnvironmentInfo
    {
        public static void LoadEnvironment()
        {
            System.Console.WriteLine("LoadEnvironment...");

            if (string.IsNullOrEmpty(System.Environment.GetEnvironmentVariable("K4A_BINARY_DIR")))
            {
                //System.Environment.SetEnvironmentVariable("K4A_BINARY_DIR", @"D:/Projects/Azure-Kinect-Sensor-SDK/build");
                System.Environment.SetEnvironmentVariable("K4A_BINARY_DIR", @"D:/Projects/Azure-Kinect-Sensor-SDK/build64");
            }

            if (string.IsNullOrEmpty(System.Environment.GetEnvironmentVariable("K4A_SOURCE_DIR")))
            {
                System.Environment.SetEnvironmentVariable("K4A_SOURCE_DIR", @"D:/Projects/Azure-Kinect-Sensor-SDK/");
            }

            if (string.IsNullOrEmpty(System.Environment.GetEnvironmentVariable("CMAKE_CXX_COMPILER")))
            {
                //System.Environment.SetEnvironmentVariable("CMAKE_CXX_COMPILER", @"C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.16.27023/bin/Hostx86/x86/cl.exe");
                System.Environment.SetEnvironmentVariable("CMAKE_CXX_COMPILER", @"C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.16.27023/bin/Hostx64/x64/cl.exe");
            }

            if (string.IsNullOrEmpty(System.Environment.GetEnvironmentVariable("CMAKE_LINKER")))
            {
                //System.Environment.SetEnvironmentVariable("CMAKE_LINKER", @"C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.16.27023/bin/Hostx86/x86/link.exe");
                System.Environment.SetEnvironmentVariable("CMAKE_LINKER", @"C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.16.27023/bin/Hostx64/x64/link.exe");
            }

            if (string.IsNullOrEmpty(System.Environment.GetEnvironmentVariable("INCLUDE")))
            {
                System.Environment.SetEnvironmentVariable("INCLUDE", @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\ATLMFC\include;C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\include;C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\include\um;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\ucrt;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\shared;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\um;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\winrt;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\cppwinrt");
            }

            if (string.IsNullOrEmpty(System.Environment.GetEnvironmentVariable("LIB")))
            {
                //System.Environment.SetEnvironmentVariable("LIB", @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\ATLMFC\lib\x86;C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\lib\x86;C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\lib\um\x86;C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\ucrt\x86;C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\um\x86;");
                System.Environment.SetEnvironmentVariable("LIB", @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\ATLMFC\lib\x64;C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\lib\x64;C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\lib\um\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\um\x64;");
            }
        }
    }
}
