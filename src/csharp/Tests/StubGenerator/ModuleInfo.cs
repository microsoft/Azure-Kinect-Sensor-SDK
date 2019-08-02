// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    internal class ModuleInfo
    {
        private ModuleInfo(string[] exports)
        {
            this.Exports = exports;
        }

        public string[] Exports { get; }

        public static ModuleInfo Analyze(string path, CompilerOptions options = null)
        {
            options = options ?? CompilerOptions.GetDefault();

            List<string> exports = new List<string>();

            if (!File.Exists(path))
            {
                throw new FileNotFoundException("Failed to analyze module, file not found.", path);
            }

            if (!options.TempPath.Exists)
            {
                options.TempPath.Create();
            }

            Regex regex = new Regex(@"^\s+\d+\s+[\dA-Fa-f]+\s+[0-9A-Fa-f]{8}\s+([^\s]*).*?$", System.Text.RegularExpressions.RegexOptions.Multiline);

            // Start the compiler process
            ProcessStartInfo startInfo = new ProcessStartInfo(options.LinkerPath.FullName)
            {
                Arguments = $"/dump \"{path}\" /exports",
                WorkingDirectory = options.TempPath.FullName,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                RedirectStandardInput = true,
            };

            try
            {
                using (Process link = Process.Start(startInfo))
                {
                    string output = link.StandardOutput.ReadToEnd().Replace("\r\n", "\n");

                    link.WaitForExit();

                    if (link.ExitCode != 0)
                    {
                        throw new AzureKinectStubGeneratorException("Link /dump failed");
                    }

                    foreach (Match m in regex.Matches(output))
                    {
                        string functionName = m.Groups[1].Value;

                        exports.Add(functionName);
                    }
                }
            }
            catch (Exception ex)
            {
                throw new AzureKinectStubGeneratorException("Failed to analyze module", ex);
            }

            return new ModuleInfo(exports.ToArray());
        }
    }
}
