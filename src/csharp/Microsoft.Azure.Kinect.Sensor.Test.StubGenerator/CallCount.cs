using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    public class CallCount
    {
        private readonly StubbedModule module;
        internal CallCount(StubbedModule module)
        {
            this.module = module;
            foreach (var function in module.NativeInterface.Functions)
            {
                initialCount.Add(function.Name, module.GetTotalCallCount(function.Name));
            }
        }

        readonly Dictionary<string, int> initialCount = new Dictionary<string, int>();

        public int Calls(string function)
        {
            return module.GetTotalCallCount(function) - initialCount[function];
        }
    }
}
