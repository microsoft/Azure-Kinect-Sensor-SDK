//------------------------------------------------------------------------------
// <copyright file="Compiler.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System.Diagnostics;
using System.IO;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    internal class Compiler
    {
        /// <summary>
        /// Compiles a DLL
        /// </summary>
        /// <param name="sourceFiles">List of source files</param>
        /// <param name="outputBinary">Path of output DLL</param>
        /// <param name="impLib">Path of output import .lib</param>
        /// <param name="options">Compiler Options</param>
        public static void CompileModule(string[] sourceFiles,
            string outputBinary,
            string impLib,
            CompilerOptions options = null,
            string additionalFlags = null,
            string[] additionalLibraries = null,
            string[] additionalSources = null)
        {
            options = options ?? CompilerOptions.GetDefault();
            string moduleName = outputBinary;

            // Set up compiler arguments
            StringBuilder compilerArguments = new StringBuilder();
            compilerArguments.Append(options.CompilerFlags);
            if (additionalFlags != null)
            {
                compilerArguments.Append(" ");
                compilerArguments.Append(additionalFlags);
            }
            compilerArguments.Append(" /nologo");
            compilerArguments.Append(" /LD"); // Create a .DLL

            foreach (string sourceFilePath in sourceFiles)
            {
                compilerArguments.Append($" \"{sourceFilePath}\"");
            }
            if (additionalSources != null)
            {
                foreach (string sourceFilePath in additionalSources)
                {
                    compilerArguments.Append($" \"{sourceFilePath}\"");
                }
            }
            foreach (DirectoryInfo includePath in options.IncludePaths)
            {
                compilerArguments.Append($" \"/I{includePath.FullName.TrimEnd('\\')}\"");
            }

            // Linker options
            compilerArguments.Append(" /link");
            foreach (DirectoryInfo libraryPath in options.LibraryPaths)
            {
                compilerArguments.Append($" \"/LIBPATH:{libraryPath.FullName.TrimEnd('\\')}\"");
            }

            compilerArguments.Append($" \"/OUT:{moduleName}\"");
            if (impLib != null)
            {
                compilerArguments.Append($" \"/IMPLIB:{impLib}\"");
            }
            foreach (string library in options.Libraries)
            {
                compilerArguments.Append(" " + library);
            }
            if (additionalLibraries != null)
            {
                foreach (string library in additionalLibraries)
                {
                    compilerArguments.Append(" " + library);
                }
            }

            // Start the compiler process
            ProcessStartInfo startInfo = new ProcessStartInfo(options.CompilerPath.FullName)
            {
                Arguments = compilerArguments.ToString(),
                WorkingDirectory = options.TempPath.FullName,
                RedirectStandardOutput = true
            };

            Debug.WriteLine(startInfo.Arguments);
            try
            {
                using (Process cl = Process.Start(startInfo))
                {
                    string output = cl.StandardOutput.ReadToEnd().Replace("\r\n", "\n");

                    cl.WaitForExit();

                    if (cl.ExitCode != 0)
                    {
                        throw new AzureKinectStubGeneratorException("Compilation failed: " + output);
                    }
                }
            }
            catch (System.ComponentModel.Win32Exception ex)
            {
                if ((uint)ex.ErrorCode == 0x80004005)
                {
                    throw new AzureKinectStubGeneratorException($"Could not find compiler \"{options.CompilerPath}\". Ensure you are running in a developer environment.", ex);
                }
                else
                {
                    throw;
                }
            }
        }
    }
}
