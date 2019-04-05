using System;
using NUnit.Framework;
using System.Runtime.InteropServices;
using System.Diagnostics;
using Microsoft.AzureKinect.Test.StubGenerator;


namespace Tests
{
#pragma warning disable IDE1006 // Naming Styles
    class TestNativeMethods
    {
        public enum k4a_result_t
        {
            K4A_RESULT_SUCCEEDED = 0,
            K4A_RESULT_FAILED,
        }

        public class k4a_device_t : Microsoft.Win32.SafeHandles.SafeHandleZeroOrMinusOneIsInvalid
        {
            private k4a_device_t() : base(true)
            {
            }

            protected override bool ReleaseHandle()
            {
                TestNativeMethods.k4a_device_close(handle);
                return true;
            }
        }

        [DllImport("k4a")]

        public static extern k4a_result_t k4a_device_open(UInt32 index, out k4a_device_t device_handle);


        [DllImport("k4a")]
        public static extern void k4a_device_close(IntPtr device_handle);

        [DllImport("k4a")]
        public static extern UInt32 k4a_device_get_installed_count();

    }
#pragma warning restore IDE1006 // Naming Styles

    public class Tests
    {
        readonly StubbedModule k4a;

        public Tests()
        {
            NativeInterface k4ainterface = NativeInterface.Create(
               @"d:\git\Azure-Kinect-Sensor-SDK\build\bin\k4a.dll",
               @"D:\git\Azure-Kinect-Sensor-SDK\include\k4a\k4a.h");

            k4a = StubbedModule.Create("k4a", k4ainterface);
        }

        [SetUp]
        public void Setup()
        {
        }

        [Test]
        public void UndefinedMethodCall()
        {
            Assert.Throws(typeof(Microsoft.AzureKinect.Test.StubGenerator.NativeFailureException), () =>
            {
                TestNativeMethods.k4a_device_get_installed_count();
            });
        }

        [Test]
        public void RedefineMethod()
        {
            k4a.SetImplementation(@"

uint32_t k4a_device_get_installed_count()
{
    return 12;
}");
            Assert.AreEqual(12, TestNativeMethods.k4a_device_get_installed_count());


            k4a.SetImplementation(@"

uint32_t k4a_device_get_installed_count()
{
    return 55;
}");
            Assert.AreEqual(55, TestNativeMethods.k4a_device_get_installed_count());
        }

        [Test]
        public void BasicMarshalling()
        {
            
            foreach (UInt32 index in new uint[] { 0, 100, 0xffffffff })
            {
                k4a.SetImplementation($@"

k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle)
{{
    STUB_ASSERT(index == { index });
    STUB_ASSERT(device_handle != NULL);

    // Assign back a fake value
    *device_handle = (k4a_device_t)0x1234ABCD; 

    return K4A_RESULT_SUCCEEDED;
}}

void k4a_device_close(k4a_device_t device_handle)
{{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
}}

");


                try
                {
                    TestNativeMethods.k4a_result_t result = TestNativeMethods.k4a_device_open(index, out TestNativeMethods.k4a_device_t handle);
                    Debug.Assert(result == TestNativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED);

                    Debug.Assert(handle.DangerousGetHandle().ToInt32() == 0x1234abcd);

                    handle.Close();
                    handle.Close();
                }
                catch
                {
                    //NativeMethods.ErrorInfo error = new NativeMethods.ErrorInfo();
                    //error.expression = "Test expression";
                    //NativeMethods.Stub_GetError(ref error);
                    //Console.WriteLine($"{error.szFile}({error.line}) : Assert failed {error.expression}");
                }
            }
        }

        [Test]
        public void STUB_FAIL()
        {
            k4a.SetImplementation(@"

uint32_t k4a_device_get_installed_count()
{
    STUB_FAIL(""method failed"");
    return 12;
}");

            Assert.Throws(typeof(Microsoft.AzureKinect.Test.StubGenerator.NativeFailureException), () =>
            {
                TestNativeMethods.k4a_device_get_installed_count();
            });

        }

        [Test]
        public void NoExports()
        {
            Assert.Throws(typeof(Exception), () =>
            {
                k4a.SetImplementation(@"

uint32_t this_function_is_not_an_export()
{
    return 12;
}");
            });
        }

        [Test]
        public void CompilationError()
        {
            Assert.Throws(typeof(Exception), () =>
            {
                k4a.SetImplementation(@"

ThisWillFailToCompile;

");
            });
        }

    }
}