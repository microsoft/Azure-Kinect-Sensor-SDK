using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    public class FunctionImplementation
    {
        internal FunctionImplementation(string name, ModuleImplementation module, IntPtr address)
        {
            this.Name = name;
            this.Module = module;
            this.Address = address;
        }

        // The reference on the ModuleImplementation keeps the module loaded
        public ModuleImplementation Module { get; }

        public string Name { get; }

        // Address is not publicly exposed to ensure that references to the function
        // keep the ModuleImplementation from being garbage collected, and thus the
        // module and function from being unloaded
        internal IntPtr Address { get;  }

        public override string ToString()
        {
            return Module.Name + ":" + Name;
        }
    }
}
