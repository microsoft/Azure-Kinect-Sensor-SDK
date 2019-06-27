// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using Microsoft.Azure.Kinect.Sensor.Test.StubGenerator;
using NUnit.Framework;

namespace Microsoft.Azure.Kinect.Sensor.UnitTests
{
    public class CalibrationFunctionTests
    {
        private readonly StubbedModule NativeK4a;

        public CalibrationFunctionTests()
        {
            NativeK4a = StubbedModule.Get("k4a");
            if (NativeK4a == null)
            {
                NativeInterface k4ainterface = NativeInterface.Create(
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.dll"),
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.h"));

                NativeK4a = StubbedModule.Create("k4a", k4ainterface);
            }
        }

        [Test]
        public void CalibrationGetFromRaw()
        {
        }
    }
}
