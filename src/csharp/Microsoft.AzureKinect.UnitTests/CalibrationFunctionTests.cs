using NUnit.Framework;
using Microsoft.AzureKinect.Test.StubGenerator;
using Microsoft.AzureKinect;

namespace Microsoft.AzureKinect.UnitTests
{

    public class CalibrationFunctionTests
    {
        readonly StubbedModule NativeK4a;

        public CalibrationFunctionTests()
        {
            NativeK4a = StubbedModule.Get("k4a");
            if (NativeK4a == null)
            {
                NativeInterface k4ainterface = NativeInterface.Create(
                    @"%K4A_BINARY_DIR%\bin\k4a.dll",
                    @"%K4A_SOURCE_DIR%\include\k4a\k4a.h");

                NativeK4a = StubbedModule.Create("k4a", k4ainterface);
            }
        }

        [Test]
        public void CalibrationGetFromRaw()
        {
            
        }
    }
}
