using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    public class FunctionInfo
    {
        public string Declaration { get; set; }
        public string ReturnType { get; set; }
        public string Name { get; set; }
        public List<string> ArgumentType { get; } = new List<string>();
        public List<string> ArgumentName { get; } = new List<string>();
    }
}
