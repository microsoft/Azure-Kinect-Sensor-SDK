// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    public class NativeInterface
    {
        public NativeInterface(FileInfo modulePath, FileInfo headerPath, IReadOnlyList<FunctionInfo> functions)
        {
            this.HeaderPath = headerPath;
            this.ModulePath = modulePath;
            this.Functions = functions;
        }

        public FileInfo HeaderPath { get; }
        public FileInfo ModulePath { get; }
        public IReadOnlyList<FunctionInfo> Functions { get; }

        /// <summary>
        /// Create a NativeInterface from a module path and a header with the function declarations
        /// </summary>
        /// <param name="modulePath"></param>
        /// <returns></returns>
        public static NativeInterface Create(FileInfo modulePath, FileInfo headerPath)
        {
            List<FunctionInfo> functions = new List<FunctionInfo>();

            // Get the exports from the module
            ModuleInfo module = ModuleInfo.Analyze(modulePath.FullName);
            List<string> exports = new List<string>(module.Exports);

            //Regex functionRegex = new Regex(@"^\s*K4A_EXPORT\s+(?<return>\S+)\s+(?<function>[a-zA-Z0-9_]+)\s*\((?<arguments>[^\)]*)\)", RegexOptions.Multiline);

            // Searches for functions of any of the following forms:
            /* 
             * void foo(int bar, obj* baz);
             *
             * extern MODULE_EXPORT void foo(int bar, obj* baz);
             *
             * __declspec(dllexport) void bar(int bar, obj* baz);
             * 
             * extern "C" MODULE_EXPORT void __stdcall fkdfjkj(int bar, obj* baz);
             * 
             */
            Regex functionRegex = new Regex(
                @"^\s*(?<prototype>(?<extern>extern(\s+""C(\+\+)?"")?\s+)?((?<premacro>[A-Z_a-z0-9]+\s+)|(?<declspec>__declspec\(\s*dllexport\s*\)\s+))?(?<return>\S+)\s+(?<pointers>\**\s*)(?<call>__[a-z]+\s+)?(?<function>[a-zA-Z0-9_]+)\s*\((?<arguments>[^\)]*)\))\s*;\s*$", RegexOptions.Multiline);

            Regex parameterRegex = new Regex(@"^\s*(?<type>.*?)(?<name>[a-zA-Z0-9_]+)\s*$");

            using (var header = File.OpenText(headerPath.FullName))
            {
                string headerData = header.ReadToEnd().Replace("\r\n", "\n");

                foreach (Match functionMatch in functionRegex.Matches(headerData))
                {
                    if (functionMatch.Groups["return"].Value == "*")
                    {
                        continue;
                    }

                    FunctionInfo info = new FunctionInfo
                    {
                        Declaration = functionMatch.Groups["prototype"].Value,
                        ReturnType = functionMatch.Groups["return"].Value
                    };
                    if (functionMatch.Groups["pointers"].Success)
                    {
                        info.ReturnType += functionMatch.Groups["pointers"].Value;
                    }
                    info.Name = functionMatch.Groups["function"].Value;

                    string[] ArgumentList = functionMatch.Groups["arguments"].Value.Split(',');

                    foreach (string arg in ArgumentList)
                    {
                        if (arg.Trim() != "void")
                        {
                            Match parameterMatch = parameterRegex.Match(arg);
                            info.ArgumentType.Add(parameterMatch.Groups["type"].Value);
                            info.ArgumentName.Add(parameterMatch.Groups["name"].Value);
                        }
                    }

                    if (exports.Contains(info.Name))
                    {
                        functions.Add(info);
                        exports.Remove(info.Name);
                    }
                    else
                    {
                        throw new AzureKinectStubGeneratorException($"Header declares a function not found in module exports. Function \"{info.Declaration}\" was not found in exports.");
                    }
                }
            }

            foreach (string export in exports)
            {
                System.Diagnostics.Debug.WriteLine($"Warning: Exported function \"{export}\" not found in header");
            }

            return new NativeInterface(modulePath, headerPath, functions.AsReadOnly());
        }

    }
}
