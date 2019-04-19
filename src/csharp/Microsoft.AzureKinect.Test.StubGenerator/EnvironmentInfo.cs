using System;
using System.Xml;

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

                    Console.WriteLine("LoadEnvironment...");

                    //string appSettingsJson = File.ReadAllText(@"appsettings.json");
                    //JObject appSettings = JObject.Parse(appSettingsJson);

                    //Environment.SetEnvironmentVariable("K4A_BINARY_DIR", appSettings["Paths"]["K4A_BINARY_DIR"].Value<string>());
                    //Environment.SetEnvironmentVariable("K4A_SOURCE_DIR", appSettings["Paths"]["K4A_SOURCE_DIR"].Value<string>());
                    //Environment.SetEnvironmentVariable("CMAKE_CXX_COMPILER", appSettings["Paths"]["CMAKE_CXX_COMPILER"].Value<string>());
                    //Environment.SetEnvironmentVariable("CMAKE_LINKER", appSettings["Paths"]["CMAKE_LINKER"].Value<string>());
                    //Environment.SetEnvironmentVariable("INCLUDE", appSettings["Paths"]["INCLUDE"].Value<string>());
                    //Environment.SetEnvironmentVariable("LIB", appSettings["Paths"]["LIB"].Value<string>());

                    XmlDocument appSettings = new XmlDocument();
                    appSettings.Load("appsettings.xml");

                    Environment.SetEnvironmentVariable("K4A_BINARY_DIR", appSettings.DocumentElement["K4A_BINARY_DIR"].InnerXml);
                    Environment.SetEnvironmentVariable("K4A_SOURCE_DIR", appSettings.DocumentElement["K4A_SOURCE_DIR"].InnerXml);
                    Environment.SetEnvironmentVariable("CMAKE_CXX_COMPILER", appSettings.DocumentElement["CMAKE_CXX_COMPILER"].InnerXml);
                    Environment.SetEnvironmentVariable("CMAKE_LINKER", appSettings.DocumentElement["CMAKE_LINKER"].InnerXml);
                    Environment.SetEnvironmentVariable("INCLUDE", appSettings.DocumentElement["INCLUDE"].InnerXml);
                    Environment.SetEnvironmentVariable("LIB", appSettings.DocumentElement["LIB"].InnerXml);

                    EnvironmentInfo.IsInitialized = true;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Failed to load settings: {0}", ex);
            }
        }
    }
}
