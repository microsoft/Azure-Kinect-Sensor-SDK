// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System.Collections.Generic;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
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
