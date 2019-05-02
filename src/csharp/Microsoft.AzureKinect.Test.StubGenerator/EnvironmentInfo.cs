using System;
using System.IO;
using System.Xml;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    public static class EnvironmentInfo
    {
        private static readonly object SyncRoot = new object();
        private static bool IsInitialized = false;

        public static void LoadEnvironment()
        {
            if (EnvironmentInfo.IsInitialized)
            {
                return;
            }

            lock (EnvironmentInfo.SyncRoot)
            {
                if (EnvironmentInfo.IsInitialized)
                {
                    return;
                }

                XmlDocument appSettings = new XmlDocument();
                appSettings.Load("appsettings.xml");

                Environment.SetEnvironmentVariable("K4A_SOURCE_DIR", appSettings.DocumentElement["K4A_SOURCE_DIR"].InnerXml);
                Environment.SetEnvironmentVariable("CMAKE_CXX_COMPILER", appSettings.DocumentElement["CMAKE_CXX_COMPILER"].InnerXml);
                Environment.SetEnvironmentVariable("CMAKE_LINKER", appSettings.DocumentElement["CMAKE_LINKER"].InnerXml);
                Environment.SetEnvironmentVariable("INCLUDE", appSettings.DocumentElement["INCLUDE"].InnerXml);
                Environment.SetEnvironmentVariable("LIB", appSettings.DocumentElement["LIB"].InnerXml);

                EnvironmentInfo.IsInitialized = true;
            }
        }

        public static FileInfo CalculateFileLocation(string filename)
        {
            EnvironmentInfo.LoadEnvironment();
            return new FileInfo(Environment.ExpandEnvironmentVariables(filename));
        }

        public static DirectoryInfo CalculateDirectoryLocation(string filename)
        {
            EnvironmentInfo.LoadEnvironment();
            return new DirectoryInfo(Environment.ExpandEnvironmentVariables(filename));
        }
    }
}
