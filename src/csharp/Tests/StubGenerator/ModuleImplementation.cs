// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    public class ModuleImplementation : IDisposable
    {
        private ModuleImplementation(string path, IntPtr hModule, IReadOnlyDictionary<string, FunctionImplementation> functions)
        {
            this.Path = path;
            this.ModuleHandle = hModule;
            this.Functions = functions;
        }

        public static ModuleImplementation Compile(CodeString code, CompilerOptions options)
        {
            Hash hashCode = Hash.GetHash(code) + Hash.GetHash(options) + Hash.GetHash("version1");
            string moduleName = System.IO.Path.Combine(options.BinaryPath.FullName, $"imp{hashCode}.dll");

            if (!System.IO.File.Exists(moduleName))
            {
                // Produce the source code file
                StringBuilder sourceFile = new StringBuilder();
                string sourceFilePath = System.IO.Path.Combine(options.TempPath.FullName, $"imp{hashCode}.cpp");

                CodeString prefix = @"
#include ""StubImplementation.h""
";
                sourceFile.AppendLine(options.CodeHeader.EmbdededLineData);
                sourceFile.AppendLine(prefix.EmbdededLineData);
                sourceFile.AppendLine(code.EmbdededLineData);
                using (var filestream = System.IO.File.CreateText(sourceFilePath))
                {
                    filestream.WriteLine(sourceFile.ToString());
                }

                string stubImportLib = System.IO.Path.Combine(options.TempPath.FullName, "stubimport.lib");

                Compiler.CompileModule(new string[] { sourceFilePath }, moduleName, null, options, additionalLibraries: new string[] { stubImportLib });
            }
            return Import(moduleName, options);
        }

        public static ModuleImplementation Import(string path, CompilerOptions options)
        {
            Dictionary<string, FunctionImplementation> functions = new Dictionary<string, FunctionImplementation>();

            // Load the module in to the current process
            IntPtr hModule = NativeMethods.LoadLibrary(path);
            if (hModule == IntPtr.Zero)
            {
                throw new AzureKinectStubGeneratorException("Unable to load implementation module");
            }


            IntPtr Stub_SetErrorFunction = NativeMethods.GetProcAddress(hModule, "Stub_SetErrorFunction");
            if (Stub_SetErrorFunction == IntPtr.Zero)
            {
                throw new AzureKinectStubGeneratorException("No Error function");
            }

            NativeMethods.Stub_SetErrorFunction setError = System.Runtime.InteropServices.Marshal.GetDelegateForFunctionPointer<NativeMethods.Stub_SetErrorFunction>(Stub_SetErrorFunction);

            setError(new NativeMethods.RaiseError(ModuleError));

            try
            {
                ModuleImplementation module = new ModuleImplementation(path, hModule, functions);

                // Inspect the module for its exports and get the function addresses
                ModuleInfo moduleInfo = ModuleInfo.Analyze(path, options);
                foreach (string export in moduleInfo.Exports)
                {
                    IntPtr functionAddress = NativeMethods.GetProcAddress(hModule, export);
                    FunctionImplementation f = new FunctionImplementation(export, module, functionAddress);
                    functions.Add(export, f);
                }

                return new ModuleImplementation(path, hModule, functions);
            }
            catch
            {
                NativeMethods.FreeLibrary(hModule);
                throw;
            }
        }

        private static void ModuleError(string file, int line, string expression)
        {
            throw new NativeFailureException(expression, file, line);
        }

        public string Path { get; }

        public string Name
        {
            get
            {
                return System.IO.Path.GetFileName(Path);
            }
        }

        public override string ToString()
        {
            return Name;
        }

        public IReadOnlyDictionary<string, FunctionImplementation> Functions { get; }

        public IntPtr ModuleHandle { get; }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                }

                NativeMethods.FreeLibrary(ModuleHandle);
                disposedValue = true;
            }
        }

        ~ModuleImplementation()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(false);
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
            GC.SuppressFinalize(this);
        }
        #endregion
    }
}
