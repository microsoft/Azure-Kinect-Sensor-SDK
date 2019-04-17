using System.IO;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    public class CompilerOptions
    {
        public static CompilerOptions defaultOptions = CompilerOptions.GenerateDefaults();
        public static CompilerOptions GetDefault()
        {
            return defaultOptions;
        }
        public static void SetDefault(CompilerOptions options)
        {
            defaultOptions = options;
        }

        public string CompilerPath { get; set; } = @"%CMAKE_CXX_COMPILER%";

        public string LinkerPath { get; set; } = @"%CMAKE_LINKER%";

        public string CompilerFlags { get; set; } = "/DWIN32 /D_WINDOWS /W3 /GR /EHa /Od /Zi";

        public string[] Libraries { get; set; } = new string[] {
            "kernel32.lib",
            "user32.lib",
            "gdi32.lib",
            "winspool.lib",
            "shell32.lib",
            "ole32.lib",
            "oleaut32.lib",
            "uuid.lib",
            "comdlg32.lib",
            "advapi32.lib"
        };
        /*
        public string[] IncludePaths { get; set; } = new string[] {
            @"D:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Tools\MSVC\14.20.27027\include",
            @"D:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Tools\MSVC\14.20.27027\atlmfc\include",
            @"D:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Auxiliary\VS\include",
            @"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\ucrt",
            @"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um",
            @"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\shared",
            @"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\winrt",
            @"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\cppwinrt",
            @"C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\Include\um",
            @"D:\git\Azure-Kinect-Sensor-SDK\include",
            @"D:\git\Azure-Kinect-Sensor-SDK\build\src\sdk\include",
            @"D:\git\Azure-Kinect-Sensor-SDK\src\csharp\K4aStub"
        };
        */

        public string[] IncludePaths { get; set; } = new string[] {
            @"%K4A_SOURCE_DIR%\include",
            @"%K4A_BINARY_DIR%\src\sdk\include",
            @"%K4A_SOURCE_DIR%\src\csharp\K4aStub"
        };

        /*
        public string[] LibraryPaths { get; set; } = new string[]
        {
            @"D:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Tools\MSVC\14.20.27027\lib\x64",
            @"D:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Tools\MSVC\14.20.27027\atlmfc\lib\x64",
            @"D:\Program Files (x86)\Microsoft Visual Studio\2019\Preview\VC\Auxiliary\VS\lib\x64",
            @"C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\ucrt\x64",
            @"C:\Program Files (x86)\Windows Kits\10\lib\10.0.17763.0\um\x64",
            @"C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\lib\um\x64",
            @"C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6.1\Lib\um\x64",
        };
        */

        public string[] LibraryPaths { get; set; } = new string[] { };

        public string BinaryPath { get; set; } = @"binaries";
        public string TempPath { get; set; } = @"compilation";

        public string StubFile { get; set; } = "Stub.cpp";
        public CodeString CodeHeader { get; set; } = @"
#define k4a_EXPORTS
#include ""k4a\k4a.h""

";

        // Gets the hash of options that may impact the output of the compiled modules
        public byte[] GetOptionsHash()
        {
            var md5 = System.Security.Cryptography.MD5.Create();

            byte[] hash = md5.ComputeHash(System.Text.Encoding.Unicode.GetBytes(
                CompilerPath + ";" +
                CompilerFlags + ";" +
                Libraries + ";" +
                IncludePaths + ";" +
                LibraryPaths
                ));

            return hash;
        }

        public CompilerOptions Copy()
        {
            return new CompilerOptions()
            {
                BinaryPath = this.BinaryPath,
                CompilerFlags = this.CompilerFlags,
                CompilerPath = this.CompilerPath,
                IncludePaths = this.IncludePaths,
                Libraries = this.Libraries,
                LibraryPaths = this.LibraryPaths,
                LinkerPath = this.LinkerPath,
                StubFile = this.StubFile,
                TempPath = this.TempPath,
                CodeHeader = this.CodeHeader
            };
        }

        private static CompilerOptions GenerateDefaults()
        {
            CompilerOptions options = new CompilerOptions();

            EnvironmentInfo.LoadEnvironment();

            for (int i = 0; i < options.IncludePaths.Length; ++i)
            {
                options.IncludePaths[i] = System.Environment.ExpandEnvironmentVariables(options.IncludePaths[i]);
            }

            options.CompilerPath = System.Environment.ExpandEnvironmentVariables(options.CompilerPath);
            options.LinkerPath = System.Environment.ExpandEnvironmentVariables(options.LinkerPath);

            string baseTempPath = Path.Combine(Path.GetTempPath(), "AzureKinect");
            options.BinaryPath = Path.Combine(baseTempPath, options.BinaryPath);
            options.TempPath = Path.Combine(baseTempPath, options.TempPath);

            return options;
        }
    }
}
