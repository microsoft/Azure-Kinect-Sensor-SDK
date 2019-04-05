using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    
       

    public class StubbedModule
    {
        public CompilerOptions CompilerOptions { get; }

        public string ModuleName { get;  }

        readonly Dictionary<string, FunctionImplementation> currentFunctionImplementations = new Dictionary<string, FunctionImplementation>();
        readonly Dictionary<Hash, ModuleImplementation> implementedModules = new Dictionary<Hash, ModuleImplementation>();

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


            string sourceFilePath = System.IO.Path.Combine(options.TempPath, "stubfunctions.cpp");
            
            using (var filestream = System.IO.File.CreateText(sourceFilePath))
            {
                filestream.WriteLine("#include \"stub.h\"");
                filestream.WriteLine($"// Defined at {options.CodeHeader.SourceFileName} line {options.CodeHeader.SourceLineNumber}");
                filestream.WriteLine(options.CodeHeader.Code);
                filestream.WriteLine($"// Auto generated from {@interface.HeaderPath}");
                filestream.WriteLine(stubCode.Code);
            }


            System.IO.File.Copy("Stub.cpp", System.IO.Path.Combine(options.TempPath, "Stub.cpp"), true);
            System.IO.File.Copy("Stub.h", System.IO.Path.Combine(options.TempPath, "Stub.h"), true);
            System.IO.File.Copy("StubImplementation.h", System.IO.Path.Combine(options.TempPath, "StubImplementation.h"), true);

            string stubImportLib = System.IO.Path.Combine(options.TempPath, "stubimport.lib");

            CompilerOptions modifiedOptions = options.Copy();
            modifiedOptions.CompilerFlags += " /DStubExport";
            
            Compiler.CompileModule(new string[] { sourceFilePath, "Stub.cpp" }, modulePath, stubImportLib, modifiedOptions);


            // Load the generated stub in to the current process. This should be done before any P/Invokes in to the
            // target module to ensure that the stub implementation is the one accessed
            _ = NativeMethods.LoadLibrary(modulePath);
        }

        public NativeInterface NativeInterface { get;  }

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
                throw new Exception("Module already stubbed");
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
                throw new Exception("Module name should not include file extension");
            }
            
            this.ModuleName = moduleName;
            this.CompilerOptions = options;
            this.NativeInterface = @interface;

            try
            {
                System.IO.Directory.Delete(options.TempPath);
                
            } catch
            {
            }
            System.IO.Directory.CreateDirectory(options.TempPath);

            try
            {
                System.IO.Directory.Delete(options.BinaryPath);   
            }
            catch
            {
            }
            System.IO.Directory.CreateDirectory(options.BinaryPath);

            string executingDirectory = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location);
            string modulePath = System.IO.Path.Combine(executingDirectory, moduleName + ".dll");
            
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
                throw new Exception("No exported functions found in implementation");
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
