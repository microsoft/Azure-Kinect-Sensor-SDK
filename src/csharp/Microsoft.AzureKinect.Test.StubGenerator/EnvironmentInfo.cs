using System;
using System.IO;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    public static class EnvironmentInfo
    {
        private static object SyncRoot = new object();
        private static bool IsInitialized = false;

        public static void LoadEnvironment()
        {
            if (EnvironmentInfo.IsInitialized)
            {
                return;
            }

            try
            {
                lock (EnvironmentInfo.SyncRoot)
                {
                    if (EnvironmentInfo.IsInitialized)
                    {
                        return;
                    }

                    System.Console.WriteLine("LoadEnvironment...");

                    string appSettingsJson = File.ReadAllText(@"appsettings.json");
                    //JObject appSettings = JObject.Parse(appSettingsJson);

                    System.Environment.SetEnvironmentVariable("K4A_BINARY_DIR", @"D:/Projects/Azure-Kinect-Sensor-SDK/build64");
                    System.Environment.SetEnvironmentVariable("K4A_SOURCE_DIR", @"D:/Projects/Azure-Kinect-Sensor-SDK/");
                    System.Environment.SetEnvironmentVariable("CMAKE_CXX_COMPILER", @"C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.16.27023/bin/Hostx64/x64/cl.exe");
                    System.Environment.SetEnvironmentVariable("CMAKE_LINKER", @"C:/Program Files (x86)/Microsoft Visual Studio/2017/Enterprise/VC/Tools/MSVC/14.16.27023/bin/Hostx64/x64/link.exe");
                    System.Environment.SetEnvironmentVariable("INCLUDE", @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\ATLMFC\include;C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\include;C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\include\um;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\ucrt;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\shared;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\um;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\winrt;C:\Program Files (x86)\Windows Kits\10\include\10.0.17763.0\cppwinrt");
                    System.Environment.SetEnvironmentVariable("LIB", @"C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\ATLMFC\lib\x64;C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Tools\MSVC\14.16.27023\lib\x64;C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\lib\um\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\um\x64;");

                    EnvironmentInfo.IsInitialized = true;
                }
            }
            catch (Exception ex)
            {
                System.Console.WriteLine("Failed to load settings: {0}", ex);
            }
        }
    }
}
