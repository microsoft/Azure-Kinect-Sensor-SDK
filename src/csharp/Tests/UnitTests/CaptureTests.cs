//------------------------------------------------------------------------------
// <copyright file="CaptureTests.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using Microsoft.Azure.Kinect.Sensor.Test.StubGenerator;
using NUnit.Framework;

namespace Microsoft.Azure.Kinect.Sensor.UnitTests
{
    /// <summary>
    /// Tests that validate the Capture object.
    /// </summary>
    public class CaptureTests
    {
        private readonly StubbedModule nativeK4a;

        /// <summary>
        /// Initializes a new instance of the <see cref="CaptureTests"/> class.
        /// </summary>
        public CaptureTests()
        {
            this.nativeK4a = StubbedModule.Get("k4a");
            if (this.nativeK4a == null)
            {
                NativeInterface k4ainterface = NativeInterface.Create(
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.dll"),
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.h"));

                this.nativeK4a = StubbedModule.Create("k4a", k4ainterface);
            }
        }

        /// <summary>
        /// Called before test methods.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            // Don't hook the native allocator
            Microsoft.Azure.Kinect.Sensor.Allocator.Singleton.UseManagedAllocator = false;
        }

        /// <summary>
        /// Validate that garbage collection closes all references to the capture.
        /// </summary>
        [Test]
        public void CaptureGarbageCollection()
        {
            this.nativeK4a.SetImplementation(@"

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

k4a_result_t k4a_set_debug_message_handler(
    k4a_logging_message_cb_t *message_cb,
    void *message_cb_context,
    k4a_log_level_t min_level)
{
    STUB_ASSERT(message_cb != NULL);

    return K4A_RESULT_SUCCEEDED;
}

");

            CallCount count = this.nativeK4a.CountCalls();

            Assert.AreEqual(0, count.Calls("k4a_capture_create"));
            Assert.AreEqual(0, count.Calls("k4a_capture_release"));

            WeakReference capture = this.CreateWithWeakReference(() =>
            {
                Capture c = new Capture();

                // The reference should still exist and we should have not seen close called
                Assert.AreEqual(1, count.Calls("k4a_capture_create"));
                Assert.AreEqual(0, count.Calls("k4a_capture_release"));

                return c;
            });

            // The reference to the Capture object is no longer on the stack, and therefore is free to be garbage collected
            // At this point capture.IsAlive is likely to be true, but not guaranteed to be

            // Force garbage collection
            GC.Collect(0, GCCollectionMode.Forced, true);
            GC.WaitForPendingFinalizers();

            Assert.AreEqual(false, capture.IsAlive);

            // k4a_capture_release should have been called automatically
            Assert.AreEqual(1, count.Calls("k4a_capture_create"));
            Assert.AreEqual(1, count.Calls("k4a_capture_release"));
        }

        /// <summary>
        /// Validate the Capture.Temperature property.
        /// </summary>
        [Test]
        public void Temperature()
        {
            this.SetCaptureReleaseImplementation();

            this.nativeK4a.SetImplementation(@"

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

            CallCount count = this.nativeK4a.CountCalls();

            using (Capture c = new Capture())
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

                // Verify the write is being marshaled correctly
                _ = Assert.Throws(typeof(NativeFailureException), () =>
                  {
                      c.Temperature = 4.0f;
                  });

                c.Dispose();

                // Verify disposed behavior
                _ = Assert.Throws(typeof(ObjectDisposedException), () =>
                  {
                      c.Temperature = 4.0f;
                  });

                _ = Assert.Throws(typeof(ObjectDisposedException), () =>
                  {
                      float x = c.Temperature;
                  });
            }
        }


        /// <summary>
        /// Verify that methods throw ObjectDisposedException after the Capture is disposed
        /// </summary>
        [Test]
        public void DisposeBehavior()
        {
            this.SetCaptureReleaseImplementation();

            Capture c = new Capture();
            c.Dispose();

            // Verify disposed behavior
            _ = Assert.Throws(typeof(ObjectDisposedException), () =>
            {
                c.Color = null;
            });

            _ = Assert.Throws(typeof(ObjectDisposedException), () =>
            {
                Image image = c.Color;
            });

            _ = Assert.Throws(typeof(ObjectDisposedException), () =>
            {
                _ = c.Reference();
            });

            _ = Assert.Throws(typeof(ObjectDisposedException), () =>
            {
                _ = c.DangerousGetHandle();
            });
        }

        /// <summary>
        /// Verify the Equals operator.
        /// </summary>
        [Test]
        public void EqualsBehavior()
        {
            this.SetCaptureReleaseImplementation();

            // Mock an implementation that allows for multiple captures
            this.nativeK4a.SetImplementation(@"

#include <malloc.h>

typedef struct 
{
    int refcount;
} mock_capture_t;

k4a_result_t k4a_capture_create(k4a_capture_t* capture_handle)
{
    mock_capture_t* capture = (mock_capture_t*)malloc(sizeof(mock_capture_t));
    capture->refcount = 1;
    *capture_handle = (k4a_capture_t)capture;
    return K4A_RESULT_SUCCEEDED;
}

void k4a_capture_reference(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle != NULL);
    mock_capture_t* capture = (mock_capture_t*)capture_handle;
    capture->refcount++;
}

void k4a_capture_release(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle != NULL);
    mock_capture_t* capture = (mock_capture_t*)capture_handle;
    capture->refcount--;
    
    STUB_ASSERT(capture->refcount >= 0);
}

");

            Capture c1 = new Capture();
            Capture c2 = new Capture();
            Capture c1_ref = c1.Reference();
            Capture c2_ref = c2.Reference();

            // Verify the IntPtr values directly
            IntPtr c1_native = c1.DangerousGetHandle().DangerousGetHandle();
            IntPtr c1_ref_native = c1_ref.DangerousGetHandle().DangerousGetHandle();
            IntPtr c2_native = c2.DangerousGetHandle().DangerousGetHandle();
            IntPtr c2_ref_native = c2_ref.DangerousGetHandle().DangerousGetHandle();

            Assert.AreNotEqual(c1_native, c2_native);
            Assert.AreEqual(c1_native, c1_ref_native);
            Assert.AreEqual(c2_native, c2_ref_native);

            // References to each other should be equal
            Assert.IsTrue(c1.NativeEquals(c1_ref));
            Assert.IsTrue(c2.NativeEquals(c2_ref));

            // All others are not
            Assert.IsFalse(c1.NativeEquals(c2));
            Assert.IsFalse(c1_ref.NativeEquals(c2_ref));
            Assert.IsFalse(c1.NativeEquals(c2_ref));
            Assert.IsFalse(c2_ref.NativeEquals(c1));

            // Verify post dispose behavior
            c1.Dispose();
            Assert.IsFalse(c1.NativeEquals(c1_ref));

            _ = Assert.Throws(typeof(ObjectDisposedException), () =>
            {
                _ = c1.DangerousGetHandle();
            });

            c1.Dispose();
            c2.Dispose();
            c1_ref.Dispose();
            c2_ref.Dispose();
        }

        /// <summary>
        /// Validate the logic behind the Image properties of the capture.
        /// </summary>
        [Test]
        public void ImageProperties()
        {
            this.SetCaptureReleaseImplementation();
            this.SetImageMockImplementation();

            this.nativeK4a.SetImplementation(@"

k4a_image_t color = 0;
k4a_image_t depth = 0;
k4a_image_t ir = 0;

k4a_image_t k4a_capture_get_color_image(k4a_capture_t capture_handle)
{
    return color;
}

void k4a_capture_set_color_image(k4a_capture_t capture_handle, k4a_image_t image)
{
    color = image;
}

k4a_image_t k4a_capture_get_depth_image(k4a_capture_t capture_handle)
{
    return depth;
}

void k4a_capture_set_depth_image(k4a_capture_t capture_handle, k4a_image_t image)
{
    depth = image;
}

k4a_image_t k4a_capture_get_ir_image(k4a_capture_t capture_handle)
{
    return ir;
}

void k4a_capture_set_ir_image(k4a_capture_t capture_handle, k4a_image_t image)
{
    ir = image;
}

");

            CallCount count = this.nativeK4a.CountCalls();

            // Verify we can create and destroy using the default constructor
            using (Capture c = new Capture())
            {
            }

            using (Capture c = new Capture())
            {
                using (Capture captureReference = c.Reference())
                {
                    // Verify we have two capture objects that represent the same
                    // native object
                    Assert.IsTrue(c.NativeEquals(captureReference));
                    Assert.AreNotSame(c, captureReference);

                    Image image1 = new Image(ImageFormat.ColorBGRA32, 10, 10);
                    Image image1_reference = image1.Reference();
                    Image image2 = new Image(ImageFormat.ColorBGRA32, 10, 10);
                    Image image2_reference = image2.Reference();

                    Assert.IsFalse(image1.NativeEquals(image2));
                    Assert.AreNotSame(image1, image2);

                    // Verify our images represent the same native object, but are not the same managed wrapper
                    Assert.IsTrue(image1.NativeEquals(image1_reference));
                    Assert.AreNotSame(image1, image1_reference);
                    Assert.IsTrue(image2.NativeEquals(image2_reference));
                    Assert.AreNotSame(image2, image2_reference);

                    Assert.IsNull(c.Color);

                    // Assign image1 to the capture, it is now owned by the capture object
                    c.Color = image1;

                    // both captures should refer to the same image
                    Assert.IsTrue(image1.NativeEquals(c.Color));
                    Assert.IsFalse(image2.NativeEquals(c.Color));
                    Assert.IsTrue(image1.NativeEquals(captureReference.Color));
                    Assert.IsFalse(image2.NativeEquals(captureReference.Color));

                    // The reference should have its own wrapper that represents
                    // the same native image
                    Assert.IsTrue(c.Color.NativeEquals(captureReference.Color));
                    Assert.AreNotSame(c.Color, captureReference.Color);

                    c.Color = image2;

                    // By re-assigning the image, the capture will dispose the original image
                    _ = Assert.Throws(typeof(ObjectDisposedException), () =>
                        {
                            _ = image1.Size;
                        });

                    // Verify that the capture is now referring to image2.
                    Assert.IsTrue(image2.NativeEquals(c.Color));
                    Assert.AreSame(image2, c.Color);

                    // Verify that the reference to the capture also is updated to the same new image
                    Assert.IsFalse(image1_reference.NativeEquals(captureReference.Color));
                    Assert.IsTrue(image2.NativeEquals(captureReference.Color));

                    // The second capture will have its own wrapper
                    Assert.AreNotSame(image2, captureReference.Color);

                    // Assign the depth and IR properties to the same image
                    c.Depth = image2;
                    c.IR = image2;

                    Assert.AreSame(image2, c.Depth);
                    Assert.AreSame(image2, c.IR);
                }
            }
        }

        private WeakReference CreateWithWeakReference<T>(Func<T> factory)
        {
            return new WeakReference(factory());
        }

        // Helper to create basic capture create/release implementation
        private void SetCaptureReleaseImplementation()
        {
            this.nativeK4a.SetImplementation(@"

static int captureRefCount = 0;

k4a_result_t k4a_capture_create(k4a_capture_t* capture_handle)
{
    STUB_ASSERT(capture_handle != NULL);
    STUB_ASSERT(captureRefCount == 0);

    *capture_handle = (k4a_capture_t)0x0C001234;
    captureRefCount = 1;
    return K4A_RESULT_SUCCEEDED;
}

void k4a_capture_reference(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle == (k4a_capture_t)0x0C001234);
    captureRefCount++;
}

void k4a_capture_release(k4a_capture_t capture_handle)
{
    STUB_ASSERT(capture_handle == (k4a_capture_t)0x0C001234);
    captureRefCount--;
}

k4a_result_t k4a_set_debug_message_handler(
    k4a_logging_message_cb_t *message_cb,
    void *message_cb_context,
    k4a_log_level_t min_level)
{
    STUB_ASSERT(message_cb != NULL);

    return K4A_RESULT_SUCCEEDED;
}

");
        }

        // Helper to create basic capture create/release implementation
        private void SetImageMockImplementation()
        {
            this.nativeK4a.SetImplementation(@"

#include <malloc.h>

typedef struct 
{
    int refcount;
} mock_image_t;

k4a_result_t k4a_image_create(k4a_image_format_t format,
                                         int width_pixels,
                                         int height_pixels,
                                         int stride_bytes,
                                         k4a_image_t *image_handle)
{
    mock_image_t* image = (mock_image_t*)malloc(sizeof(mock_image_t));
    image->refcount = 1;
    *image_handle = (k4a_image_t)image;
    return K4A_RESULT_SUCCEEDED;
}

void k4a_image_reference(k4a_image_t image_handle)
{
    mock_image_t* image = (mock_image_t*)image_handle;

    image->refcount++;
}

void k4a_image_release(k4a_image_t image_handle)
{
    mock_image_t* image = (mock_image_t*)image_handle;

    image->refcount--;

    // Refcounts will go below zero since the mock doesn't properly
    // implement the references that k4a_capture_get_*_image should add.
    // STUB_ASSERT(image->refcount >= 0);
}

");
        }
    }
}
