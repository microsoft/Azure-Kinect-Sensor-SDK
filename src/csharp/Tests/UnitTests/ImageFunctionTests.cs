// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using Microsoft.Azure.Kinect.Sensor.Test.StubGenerator;
using NUnit.Framework;
using System;
using System.Buffers;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace Microsoft.Azure.Kinect.Sensor.UnitTests
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
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.dll"),
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.h"));

                NativeK4a = StubbedModule.Create("k4a", k4ainterface);
            }
        }

        [SetUp]
        public void Setup()
        {
            // Force garbage collection
            System.GC.Collect(0, System.GCCollectionMode.Forced, true);
            System.GC.WaitForPendingFinalizers();

            // Don't hook the native allocator
            Microsoft.Azure.Kinect.Sensor.Allocator.Singleton.UseManagedAllocator = false;
        }

        // Helper function to implement basic create/release behavior
        private void SetImageStubImplementation()
        {
            NativeK4a.SetImplementation(@"

uint16_t dummybuffer[640*480];

int referenceCount = 0;

k4a_result_t k4a_image_create(k4a_image_format_t format, int width_pixels, int height_pixels, int stride_bytes, k4a_image_t* image_handle)
{
    STUB_ASSERT(image_handle != NULL);

    STUB_ASSERT(format == K4A_IMAGE_FORMAT_CUSTOM);
    STUB_ASSERT(width_pixels == 640);
    STUB_ASSERT(height_pixels == 480);
    STUB_ASSERT(stride_bytes == (640*2));

    *image_handle = (k4a_image_t)0x0D001234;

    for (int i = 0; i < 640 * 480; i++)
    {
        dummybuffer[i] = (uint16_t)i; 
    }

    //STUB_ASSERT(referenceCount == 0);
    referenceCount = 1;

    return K4A_RESULT_SUCCEEDED;
}

size_t k4a_image_get_size(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 640*2*480;
}

uint8_t* k4a_image_get_buffer(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return (uint8_t*)dummybuffer; 
}

int k4a_image_get_stride_bytes(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 640*2;
}

int k4a_image_get_width_pixels(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 640;
}

int k4a_image_get_height_pixels(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);

    return 480;
}

void k4a_image_reference(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);
    referenceCount++;
}

void k4a_image_release(k4a_image_t image_handle)
{
    STUB_ASSERT(image_handle == (k4a_image_t)0x0D001234);
    referenceCount--;
    if (referenceCount == 0)
    {
        memset(dummybuffer, 0, sizeof(dummybuffer));
    }
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
            SetImageStubImplementation();

            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_image_create"));
            Assert.AreEqual(0, count.Calls("k4a_image_release"));

            System.WeakReference image = CreateWithWeakReference(() =>
            {
                Image i = new Image(ImageFormat.Custom, 640, 480, 640 * 2);

                var memory = i.Memory;

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
            Assert.AreEqual(count.Calls("k4a_image_reference") + 1, count.Calls("k4a_image_release"));
        }


  
        [Test]
        public void GetBufferCopyTest()
        {
            SetImageStubImplementation();
            
            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_image_create"));
            Assert.AreEqual(0, count.Calls("k4a_image_release"));

            using (Image image = new Image(ImageFormat.Custom, 640, 480, 640 * 2))
            {

                byte[] buffer = image.GetBufferCopy();
                Assert.AreEqual(640 * 480 * 2, image.Size);

                // k4a_image_create should have been called once 
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

                Assert.Throws(typeof(AzureKinectException), () => {
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


        [Test]
        public void ImageMemoryTest()
        {
            SetImageStubImplementation();
            
            CallCount count = NativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_image_create"));
            Assert.AreEqual(0, count.Calls("k4a_image_release"));

            Task.Run(() =>
            {
                using (Image image = new Image(ImageFormat.Custom, 640, 480, 640 * 2))
                {
                    Memory<byte> memory = image.Memory;
                    System.Span<byte> memorySpan = memory.Span;

                    System.Span<short> shortSpan = MemoryMarshal.Cast<byte, short>(memorySpan);

                    Assert.AreEqual(0, shortSpan[0]);
                    Assert.AreEqual(1, shortSpan[1]);
                    image.Dispose();
                }
            }).Wait();

            GC.Collect(0, GCCollectionMode.Forced, true, true);
            GC.WaitForPendingFinalizers();
        
            Assert.AreEqual(count.Calls("k4a_image_reference") + 1, count.Calls("k4a_image_release"), "References not zero");
        }
    }
}
