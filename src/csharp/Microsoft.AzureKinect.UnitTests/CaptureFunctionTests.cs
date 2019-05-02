using Microsoft.AzureKinect.Test.StubGenerator;
using NUnit.Framework;

namespace Microsoft.AzureKinect.UnitTests
{
    public class CaptureFunctionTests
    {
        private readonly StubbedModule NativeK4a;

        public CaptureFunctionTests()
        {
            NativeK4a = StubbedModule.Get("k4a");
            if (NativeK4a == null)
            {
                NativeInterface k4ainterface = NativeInterface.Create(
                    EnvironmentInfo.CalculateFileLocation(@"k4a.dll"),
                    EnvironmentInfo.CalculateFileLocation(@"%K4A_SOURCE_DIR%\include\k4a\k4a.h"));

                NativeK4a = StubbedModule.Create("k4a", k4ainterface);
            }
        }

        [SetUp]
        public void Setup()
        {

        }

        // Helper function to implement basic open/close behavior
        private void SetOpenCloseImplementation()
        {
            NativeK4a.SetImplementation(@"

k4a_result_t k4a_device_open(uint32_t index, k4a_device_t *device_handle)
{{
    STUB_ASSERT(index == 0);
    STUB_ASSERT(device_handle != NULL);

    // Assign back a fake value
    *device_handle = (k4a_device_t)0x1234ABCD; 

    return K4A_RESULT_SUCCEEDED;
}}

void k4a_device_close(k4a_device_t device_handle)
{{
    STUB_ASSERT(device_handle == (k4a_device_t)0x1234ABCD);
}}");
        }

        // Helper to create basic capture create/release implementation
        private void SetCaptureReleaseImplementation()
        {
            NativeK4a.SetImplementation(@"

k4a_result_t k4a_capture_create(k4a_capture_t* capture_handle)
{
    STUB_ASSERT(capture_handle != NULL);

    *capture_handle = (k4a_capture_t)0x0C001234;
    return K4A_RESULT_SUCCEEDED;
}

void k4a_capture_reference(k4a_capture_t capture_handle)
{
    STUB_FAIL(""Can't reference this dummy capture"");
}

void k4a_capture_release(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle == (k4a_capture_t)0x0C001234);
}
");
        }

        private System.WeakReference CreateWithWeakReference<T>(System.Func<T> factory)
        {
            return new System.WeakReference(factory());
        }

        // Validate that garbage collection calls the handle close function by way of the finalizer
        [Test]
        public void CaptureGarbageCollection()
        {
            SetOpenCloseImplementation();
            NativeK4a.SetImplementation(@"

k4a_result_t k4a_capture_create(k4a_capture_t* capture_handle)
{
    STUB_ASSERT(capture_handle != NULL);

    *capture_handle = (k4a_capture_t)0x0C001234;
    return K4A_RESULT_SUCCEEDED;
}

void k4a_capture_release(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle == (k4a_capture_t)0x0C001234);
}

");

            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_capture_create"));
            Assert.AreEqual(0, count.Calls("k4a_capture_release"));

            System.WeakReference capture = CreateWithWeakReference(() =>
            {
                Capture c = Capture.Create();

                // The reference should still exist and we should have not seen close called
                Assert.AreEqual(1, count.Calls("k4a_capture_create"));
                Assert.AreEqual(0, count.Calls("k4a_capture_release"));

                return c;
            });
            // The reference to the Device object is no longer on the stack, and therefore is free to be garbage collected
            // At this point capture.IsAlive is likely to be true, but not garanteed to be

            // Force garbage collection
            System.GC.Collect(0, System.GCCollectionMode.Forced, true);
            System.GC.WaitForPendingFinalizers();


            Assert.AreEqual(false, capture.IsAlive);

            // k4a_device_close should have been called automatically 
            Assert.AreEqual(1, count.Calls("k4a_capture_create"));
            Assert.AreEqual(1, count.Calls("k4a_capture_release"));
        }

        [Test]
        public void CaptureTemperature()
        {
            SetOpenCloseImplementation();
            SetCaptureReleaseImplementation();

            NativeK4a.SetImplementation(@"

float k4a_capture_get_temperature_c(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle == (k4a_capture_t)0x0C001234);

    return 2.5f; 
}

void k4a_capture_set_temperature_c(k4a_capture_t capture_handle, float value)
{
    STUB_ASSERT(capture_handle == (k4a_capture_t)0x0C001234);

    STUB_ASSERT(value == 3.5f);
}

");

            CallCount count = NativeK4a.CountCalls();

            using (Capture c = Capture.Create())
            {
                // Temperature values should not be cached, so every access should call the
                // native layer
                Assert.AreEqual(0, count.Calls("k4a_capture_get_temperature_c"));
                Assert.AreEqual(2.5f, c.Temperature);
                Assert.AreEqual(1, count.Calls("k4a_capture_get_temperature_c"));
                Assert.AreEqual(2.5f, c.Temperature);
                Assert.AreEqual(2, count.Calls("k4a_capture_get_temperature_c"));

                // Verify writes
                Assert.AreEqual(0, count.Calls("k4a_capture_set_temperature_c"));
                c.Temperature = 3.5f;
                Assert.AreEqual(1, count.Calls("k4a_capture_set_temperature_c"));

                // Verify the write is being marshalled correctly
                Assert.Throws(typeof(NativeFailureException), () =>
                {
                    c.Temperature = 4.0f;
                });

                c.Dispose();

                // Verify disposed behavior
                Assert.Throws(typeof(System.ObjectDisposedException), () =>
                {
                    c.Temperature = 4.0f;
                });

                Assert.Throws(typeof(System.ObjectDisposedException), () =>
                {
                    float x = c.Temperature;
                });
            }
        }


    }
}
