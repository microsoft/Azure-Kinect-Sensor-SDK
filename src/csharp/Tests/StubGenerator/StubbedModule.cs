// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    public class StubbedModule
    {
        public CompilerOptions CompilerOptions { get; }

        public string ModuleName { get; }

        private readonly Dictionary<string, FunctionImplementation> currentFunctionImplementations = new Dictionary<string, FunctionImplementation>();
        private readonly Dictionary<Hash, ModuleImplementation> implementedModules = new Dictionary<Hash, ModuleImplementation>();

        private void GenerateStub(string modulePath, NativeInterface @interface, CompilerOptions options)
        {
            // Generate the stub function definitions
            CodeString stubCode = new CodeString(options.CodeHeader);
            foreach (FunctionInfo def in @interface.Functions)
            {
                string templatePrototype = def.ReturnType + "(" + string.Join(",", def.ArgumentType) + ")";

                StringBuilder stubFunction = new StringBuilder();
                stubFunction.AppendLine(def.Declaration);
                stubFunction.AppendLine("{");
                string passthroughArguments = string.Join(", ", (new string[] { $"\"{def.Name}\"" }).Concat(def.ArgumentName));
                string functionReturn = def.ReturnType == "void" ? "" : "return ";
                stubFunction.AppendLine($"    Stub_RecordCall(\"{def.Name}\");");
                stubFunction.AppendLine($"    {functionReturn}redirect<{templatePrototype}, {def.ReturnType}>({passthroughArguments});");
                stubFunction.AppendLine("}");
                stubFunction.AppendLine();

                stubCode += stubFunction.ToString();
            }

            string sourceFilePath = Path.Combine(options.TempPath.FullName, "stubfunctions.cpp");

            using (var filestream = File.CreateText(sourceFilePath))
            {
                filestream.WriteLine("#include \"stub.h\"");
                filestream.WriteLine($"// Defined at {options.CodeHeader.SourceFileName} line {options.CodeHeader.SourceLineNumber}");
                filestream.WriteLine(options.CodeHeader.Code);
                filestream.WriteLine($"// Auto generated from {@interface.HeaderPath}");
                filestream.WriteLine(stubCode.Code);
            }

            options.StubFile.CopyTo(Path.Combine(options.TempPath.FullName, "Stub.cpp"), true);
            System.IO.File.Copy("Stub.h", Path.Combine(options.TempPath.FullName, "Stub.h"), true);
            System.IO.File.Copy("StubImplementation.h", Path.Combine(options.TempPath.FullName, "StubImplementation.h"), true);

            string stubImportLib = Path.Combine(options.TempPath.FullName, "stubimport.lib");

            CompilerOptions modifiedOptions = options.Copy();
            modifiedOptions.CompilerFlags += " /DStubExport";

            Compiler.CompileModule(new string[] { sourceFilePath, "Stub.cpp" }, modulePath, stubImportLib, modifiedOptions);


            // Load the generated stub in to the current process. This should be done before any P/Invokes in to the
            // target module to ensure that the stub implementation is the one accessed
            _ = NativeMethods.LoadLibrary(modulePath);
        }

        public NativeInterface NativeInterface { get; }

        private static readonly Dictionary<string, StubbedModule> stubbedModules = new Dictionary<string, StubbedModule>();
        public static StubbedModule Get(string moduleName)
        {
            if (!stubbedModules.ContainsKey(moduleName))
                return null;

            return stubbedModules[moduleName];
        }
        public static StubbedModule Create(string moduleName, NativeInterface @interface, CompilerOptions options = null)
        {
            if (stubbedModules.ContainsKey(moduleName))
            {
                throw new AzureKinectStubGeneratorException("Module already stubbed");
            }
            else
            {
                StubbedModule m = new StubbedModule(moduleName, @interface, options);
                stubbedModules[moduleName] = m;
                return m;
            }
        }

        private StubbedModule(string moduleName, NativeInterface @interface, CompilerOptions options = null)
        {
            options = options ?? CompilerOptions.GetDefault();
            if (moduleName.EndsWith(".dll"))
            {
                throw new AzureKinectStubGeneratorException("Module name should not include file extension");
            }

            this.ModuleName = moduleName;
            this.CompilerOptions = options;
            this.NativeInterface = @interface;

            if (options.TempPath.Exists)
            {
                options.TempPath.Delete(true);
                options.TempPath.Refresh();
            }

            if (options.BinaryPath.Exists)
            {
                options.BinaryPath.Delete(true);
                options.BinaryPath.Refresh();
            }

            options.TempPath.Create();
            options.TempPath.Refresh();
            options.BinaryPath.Create();
            options.BinaryPath.Refresh();

            string executingDirectory = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
            string modulePath = Path.Combine(executingDirectory, moduleName + ".dll");

            GenerateStub(modulePath, @interface, options);
        }

        internal int GetTotalCallCount(string function)
        {
            return NativeMethods.Stub_GetCallCount(function);
        }

        public void SetImplementation(CodeString code)
        {
            Hash hash = Hash.GetHash(code) + Hash.GetHash(CompilerOptions);
            if (!implementedModules.ContainsKey(hash))
            {
                implementedModules.Add(hash, ModuleImplementation.Compile(code, CompilerOptions));
            }

            ModuleImplementation i = implementedModules[hash];

            if (i.Functions.Where((x) => { return !x.Value.Name.StartsWith("Stub_"); }).Count() == 0)
            {
                throw new AzureKinectStubGeneratorException("No exported functions found in implementation");
            }
            foreach (var function in i.Functions)
            {
                SetImplementation(function.Key, function.Value);
            }
        }

        public void SetImplementation(string function, FunctionImplementation implementation)
        {
            this.currentFunctionImplementations[function] = implementation;
            NativeMethods.Stub_RegisterRedirect(function, implementation.Address);
        }

        public CallCount CountCalls()
        {
            return new CallCount(this);
        }
    }
}
