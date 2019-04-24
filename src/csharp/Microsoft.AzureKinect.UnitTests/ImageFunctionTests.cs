using Microsoft.AzureKinect.Test.StubGenerator;
using NUnit.Framework;


namespace Microsoft.AzureKinect.UnitTests
{
    public class ImageFunctionTests
    {
        private readonly StubbedModule NativeK4a;

        public ImageFunctionTests()
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

        [SetUp]
        public void Setup()
        {
            // Force garbage collection
            System.GC.Collect(0, System.GCCollectionMode.Forced, true);
            System.GC.WaitForPendingFinalizers();
        }

        // Helper function to implement basic create/release behavior
        private void SetCreateReleaseImplementation()
        {
            NativeK4a.SetImplementation(@"

k4a_result_t k4a_image_create(k4a_image_format_t format, int width_pixels, int height_pixels, int stride_bytes, k4a_image_t* image_handle)
{
    STUB_ASSERT(image_handle != NULL);

    STUB_ASSERT(format == K4A_IMAGE_FORMAT_CUSTOM);
    STUB_ASSERT(width_pixels == 640);
    STUB_ASSERT(height_pixels == 480);
    STUB_ASSERT(stride_bytes == (640*2));

    *image_handle = (k4a_image_t)0x0D001234;
    return K4A_RESULT_SUCCEEDED;
}

void k4a_image_release(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);
}

size_t k4a_image_get_size(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 640*2*480;
}

");
        }

        private System.WeakReference CreateWithWeakReference<T>(System.Func<T> factory)
        {
            return new System.WeakReference(factory());
        }

        // Validate that garbage collection calls the handle close function by way of the finalizer
        [Test]
        public void ImageGarbageCollection()
        {
            SetCreateReleaseImplementation();

            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_image_create"));
            Assert.AreEqual(0, count.Calls("k4a_image_release"));

            System.WeakReference image = CreateWithWeakReference(() =>
            {
                Image i = new Image(ImageFormat.Custom, 640, 480, 640 * 2);

                // The reference should still exist and we should have not seen close called
                Assert.AreEqual(1, count.Calls("k4a_image_create"));
                Assert.AreEqual(0, count.Calls("k4a_image_release"));

                return i;
            });
            // The reference to the Device object is no longer on the stack, and therefore is free to be garbage collected
            // At this point capture.IsAlive is likely to be true, but not garanteed to be

            // Force garbage collection
            System.GC.Collect(0, System.GCCollectionMode.Forced, true);
            System.GC.WaitForPendingFinalizers();


            Assert.AreEqual(false, image.IsAlive);

            // k4a_device_close should have been called automatically 
            Assert.AreEqual(1, count.Calls("k4a_image_create"));
            Assert.AreEqual(1, count.Calls("k4a_image_release"));
        }

        [Test]
        public void GetBufferCopyTest()
        {
            SetCreateReleaseImplementation();

            NativeK4a.SetImplementation(@"

uint16_t dummybuffer[640*480];


size_t k4a_image_get_size(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 640*2*480;
}

uint8_t* k4a_image_get_buffer(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    for (int i = 0; i < 640 * 480; i++)
    {
        dummybuffer[i] = (uint16_t)i; 
    }

    return (uint8_t*)dummybuffer; 
}

");
            

            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_image_create"));
            Assert.AreEqual(0, count.Calls("k4a_image_release"));

            using (Image image = new Image(ImageFormat.Custom, 640, 480, 640 * 2))
            {

                byte[] buffer = image.GetBufferCopy();
                Assert.AreEqual(640 * 480 * 2, image.Size);

                // k4a_device_close should have been called automatically 
                Assert.AreEqual(1, count.Calls("k4a_image_create"));

                for (int i = 0; i < buffer.Length / 2; i++)
                {
                    Assert.AreEqual((ushort)i, System.BitConverter.ToUInt16(buffer, i * 2));
                }

                // Verify that writes to the buffer are to a copy and don't impact future callers
                buffer[0] = 99;

                byte[] buffer2 = image.GetBufferCopy();
                Assert.AreEqual(0, buffer2[0]);
            }


            NativeK4a.SetImplementation(@"

size_t k4a_image_get_size(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 640*2*480;
}

uint8_t* k4a_image_get_buffer(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return (uint8_t*)NULL; 
}

");
            using (Image image = new Image(ImageFormat.Custom, 640, 480, 640 * 2))
            {

                Assert.Throws(typeof(Exception), () => {
                    byte[] buffer = image.GetBufferCopy();
                });
            }

            NativeK4a.SetImplementation(@"

uint16_t dummybuffer[640*480];


size_t k4a_image_get_size(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 0;
}

uint8_t* k4a_image_get_buffer(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    for (int i = 0; i < 640 * 480; i++)
    {
        dummybuffer[i] = (uint16_t)i; 
    }

    return (uint8_t*)dummybuffer; 
}

");

            using (Image image = new Image(ImageFormat.Custom, 640, 480, 640 * 2))
            {

                byte[] buffer = image.GetBufferCopy();

                Assert.AreEqual(0, buffer.Length);

                image.Dispose();

                Assert.Throws(typeof(System.ObjectDisposedException), ()=> 
                    {
                    image.GetBufferCopy();
                });
            }
        }
    }


}
