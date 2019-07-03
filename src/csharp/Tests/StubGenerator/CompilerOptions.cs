// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System.IO;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    public class CompilerOptions
    {
        private static CompilerOptions defaultOptions = new CompilerOptions();

        public CompilerOptions()
        {
            this.CompilerPath = EnvironmentInfo.CalculateFileLocation(@"%CMAKE_CXX_COMPILER%");
            this.LinkerPath = EnvironmentInfo.CalculateFileLocation(@"%CMAKE_LINKER%");
            this.CompilerFlags = "/DWIN32 /D_WINDOWS /W3 /GR /EHa /Od /Zi";

            this.Libraries = new string[]
                {
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

            this.IncludePaths = new DirectoryInfo[]
                {
                    EnvironmentInfo.CalculateDirectoryLocation(@".\"),
                    EnvironmentInfo.CalculateDirectoryLocation(@".\stub")
                };

            this.LibraryPaths = new DirectoryInfo[] { };

            string baseTempPath = Path.Combine(Path.GetTempPath(), "AzureKinect");
            this.BinaryPath = new DirectoryInfo(Path.Combine(baseTempPath, @"binaries"));
            this.TempPath = new DirectoryInfo(Path.Combine(baseTempPath, @"compilation"));

            this.StubFile = new FileInfo(@".\stub\Stub.cpp");
        }

        private CompilerOptions(CompilerOptions other)
        {
            this.BinaryPath = other.BinaryPath;
            this.CompilerFlags = other.CompilerFlags;
            this.CompilerPath = other.CompilerPath;
            this.IncludePaths = other.IncludePaths;
            this.Libraries = other.Libraries;
            this.LibraryPaths = other.LibraryPaths;
            this.LinkerPath = other.LinkerPath;
            this.StubFile = other.StubFile;
            this.TempPath = other.TempPath;
            this.CodeHeader = other.CodeHeader;
        }

        public FileInfo CompilerPath { get; set; }

        public FileInfo LinkerPath { get; set; }

        public string CompilerFlags { get; set; }

        public string[] Libraries { get; set; }

        public DirectoryInfo[] IncludePaths { get; set; }

        public DirectoryInfo[] LibraryPaths { get; set; }

        public DirectoryInfo BinaryPath { get; set; }

        public DirectoryInfo TempPath { get; set; }

        public FileInfo StubFile { get; set; }

        public CodeString CodeHeader { get; set; } = @"
#define k4a_EXPORTS
#include ""k4a\k4a.h""

";

        public static CompilerOptions GetDefault()
        {
            return defaultOptions;
        }

        public static void SetDefault(CompilerOptions options)
        {
            defaultOptions = options;
        }

        // Gets the hash of options that may impact the output of the compiled modules
        public byte[] GetOptionsHash()
        {
            StringBuilder summaryString = new StringBuilder();

            var md5 = System.Security.Cryptography.MD5.Create();

            summaryString.AppendFormat("{0};", this.CompilerPath.FullName);
            summaryString.AppendFormat("{0};", this.LinkerPath.FullName);
            summaryString.AppendFormat("{0};", this.CompilerFlags);

            foreach (string library in this.Libraries)
            {
                summaryString.AppendFormat("{0};", library);
            }

            foreach (DirectoryInfo directory in this.IncludePaths)
            {
                summaryString.AppendFormat("{0};", directory.FullName);
            }

            foreach (DirectoryInfo directory in this.LibraryPaths)
            {
                summaryString.AppendFormat("{0};", directory.FullName);
            }

            byte[] hash = md5.ComputeHash(Encoding.Unicode.GetBytes(summaryString.ToString()));
            return hash;
        }

        public CompilerOptions Copy()
        {
            return new CompilerOptions(this);
        }
    }
}
