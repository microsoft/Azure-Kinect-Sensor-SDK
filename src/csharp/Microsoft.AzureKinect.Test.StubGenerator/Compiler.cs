using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.Linq;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    class Compiler
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
            options = options != null ? options : CompilerOptions.GetDefault();
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
            foreach (string includePath in options.IncludePaths)
            {
                compilerArguments.Append($" \"/I{includePath}\"");
            }

            // Linker options
            compilerArguments.Append(" /link");
            foreach (string libraryPath in options.LibraryPaths)
            {
                compilerArguments.Append($" \"/LIBPATH:{libraryPath}\"");
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
            ProcessStartInfo startInfo = new ProcessStartInfo(options.CompilerPath);
            startInfo.Arguments = compilerArguments.ToString();
            startInfo.WorkingDirectory = options.TempPath;

            startInfo.RedirectStandardOutput = true;

            Debug.WriteLine(startInfo.Arguments);
            try
            {
                using (Process cl = Process.Start(startInfo))
                {
                    string output = cl.StandardOutput.ReadToEnd().Replace("\r\n", "\n");

                    cl.WaitForExit();

                    if (cl.ExitCode != 0)
                    {
                        throw new Exception("Compilation failed: " + output);
                    }
                }
            } catch (System.ComponentModel.Win32Exception ex)
            {
                if ((uint)ex.ErrorCode == 0x80004005)
                {
                    throw new Exception($"Could not find compiler \"{options.CompilerPath}\". Ensure you are running in a developer environment.");
                }
                else
                {
                    throw;
                }
            }
        }
    }
}
