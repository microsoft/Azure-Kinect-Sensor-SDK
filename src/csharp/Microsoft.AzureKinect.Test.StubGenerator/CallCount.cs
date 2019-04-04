using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    public class CallCount
    {
        private StubbedModule module;
        internal CallCount(StubbedModule module)
        {
            this.module = module;
            foreach (var function in module.NativeInterface.Functions)
            {
                initialCount.Add(function.Name, module.GetTotalCallCount(function.Name));
            }
        }

        Dictionary<string, int> initialCount = new Dictionary<string, int>();

        public int Calls(string function)
        {
            return module.GetTotalCallCount(function) - initialCount[function];
        }
    }
}
