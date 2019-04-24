using System;
using System.Collections.Generic;
using System.Diagnostics;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    internal class ModuleInfo
    {
        private ModuleInfo(string[] exports)
        {
            this.Exports = exports;
        }

        public static ModuleInfo Analyze(string path, CompilerOptions options = null)
        {
            options = options ?? CompilerOptions.GetDefault();

            List<string> exports = new List<string>();

            if (!System.IO.File.Exists(path))
            {
                throw new System.IO.FileNotFoundException("Failed to analyze module, file not found.", path);
            }

            if (!options.TempPath.Exists)
            {
                options.TempPath.Create();
            }

            var regex = new System.Text.RegularExpressions.Regex(@"^\s+\d+\s+[\dA-Fa-f]+\s+[0-9A-Fa-f]{8}\s+([^\s]*).*?$", System.Text.RegularExpressions.RegexOptions.Multiline);
            // Start the compiler process
            ProcessStartInfo startInfo = new ProcessStartInfo(options.LinkerPath.FullName)
            {
                Arguments = $"/dump \"{path}\" /exports",
                WorkingDirectory = options.TempPath.FullName,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                RedirectStandardInput = true
            };
            try
            {
                using (Process link = Process.Start(startInfo))
                {
                    string output = link.StandardOutput.ReadToEnd().Replace("\r\n", "\n");

                    link.WaitForExit();

                    if (link.ExitCode != 0)
                    {
                        throw new Exception("Link /dump failed");
                    }

                    foreach (System.Text.RegularExpressions.Match m in regex.Matches(output))
                    {
                        string functionName = m.Groups[1].Value;

                        exports.Add(functionName);


                    }

                }
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine(ex, "exception info");
                throw new InvalidOperationException("Failed to analyze module", ex);
            }

            return new ModuleInfo(exports.ToArray());
        }

        public string[] Exports { get; }
    }
}
