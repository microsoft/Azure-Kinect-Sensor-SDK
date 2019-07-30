// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// A set of images captured in sync.
    /// </summary>
    public class Capture : IDisposable
    {
        // Image caches
        // "Public" Images are instances available outside this class via properties.
        // "Private" Images are instances held within this class to ensure they can't be disposed when
        // this class still needs access to their data.
        private Image cachedPublicColor;
        private Image cachedPrivateColor;
        private Image cachedPublicDepth;
        private Image cachedPrivateDepth;
        private Image cachedPublicIR;
        private Image cachedPrivateIR;

        private NativeMethods.k4a_capture_t handle;

        private bool disposedValue = false; // To detect redundant calls

        /// <summary>
        /// Initializes a new instance of the <see cref="Capture"/> class.
        /// </summary>
        public Capture()
        {
#pragma warning disable CA2000 // Dispose objects before losing scope
            AzureKinectException.ThrowIfNotSuccess(
                NativeMethods.k4a_capture_create(out this.handle));
#pragma warning restore CA2000 // Dispose objects before losing scope

            // Hook the native allocator and register this object.
            // .Dispose() will be called on this object when the allocator is shut down.
            Allocator.Singleton.RegisterForDisposal(this);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Capture"/> class.
        /// </summary>
        /// <param name="handle">Native handle. The new class takes ownership.</param>
        internal Capture(NativeMethods.k4a_capture_t handle)
        {
            // Hook the native allocator and register this object.
            // .Dispose() will be called on this object when the allocator is shut down.
            Allocator.Singleton.RegisterForDisposal(this);

            this.handle = handle;
        }

        /// <summary>
        /// Gets or sets the color image of this capture.
        /// </summary>
        /// <remarks>
        /// Images assigned to this capture are owned by the Capture. When the Capture is disposed, the
        /// Color Image will be disposed.
        ///
        /// By setting the Color image on a Capture, the Capture takes ownership and the Capture will
        /// dispose the Image when the Capture is disposed.
        ///
        /// To get an instance of an Image that lives longer than the capture, call Image.Reference().
        /// </remarks>
        public Image Color
        {
            get
            {
                this.GetImageWrapperAndDisposePrevious(NativeMethods.k4a_capture_get_color_image, ref this.cachedPublicColor, ref this.cachedPrivateColor);

                return this.cachedPublicColor;
            }

            set
            {
                this.SetImageWrapperAndDisposePrevious(NativeMethods.k4a_capture_set_color_image, ref this.cachedPublicColor, ref this.cachedPrivateColor, value);
            }
        }

        /// <summary>
        /// Gets or sets the depth image of this capture.
        /// </summary>
        /// <remarks>
        /// Images assigned to this capture are owned by the Capture. When the Capture is disposed, the
        /// Depth Image will be disposed.
        ///
        /// By setting the Depth image on a Capture, the Capture takes ownership and the Capture will
        /// dispose the Image when the Capture is disposed.
        ///
        /// To get an instance of an Image that lives longer than the capture, call Image.Reference().
        /// </remarks>
        public Image Depth
        {
            get
            {
                this.GetImageWrapperAndDisposePrevious(NativeMethods.k4a_capture_get_depth_image, ref this.cachedPublicDepth, ref this.cachedPrivateDepth);

                return this.cachedPublicDepth;
            }

            set
            {
                this.SetImageWrapperAndDisposePrevious(NativeMethods.k4a_capture_set_depth_image, ref this.cachedPublicDepth, ref this.cachedPrivateDepth, value);
            }
        }

        /// <summary>
        /// Gets or sets the IR image of this capture.
        /// </summary>
        /// <remarks>
        /// Images assigned to this capture are owned by the Capture. When the Capture is disposed, the
        /// IR Image will be disposed.
        ///
        /// By setting the IR image on a Capture, the Capture takes ownership and the Capture will
        /// dispose the Image when the Capture is disposed.
        ///
        /// To get an instance of an Image that lives longer than the capture, call Image.Reference().
        /// </remarks>
        public Image IR
        {
            get
            {
                this.GetImageWrapperAndDisposePrevious(NativeMethods.k4a_capture_get_ir_image, ref this.cachedPublicIR, ref this.cachedPrivateIR);

                return this.cachedPublicIR;
            }

            set
            {
                this.SetImageWrapperAndDisposePrevious(NativeMethods.k4a_capture_set_ir_image, ref this.cachedPublicIR, ref this.cachedPrivateIR, value);
            }
        }

        /// <summary>
        /// Gets or sets the device temperature of the capture.
        /// </summary>
        /// <remarks>
        /// Temperature is represented in degrees Celcius.
        /// </remarks>
        public float Temperature
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Capture));
                    }

                    return NativeMethods.k4a_capture_get_temperature_c(this.handle);
                }
            }

            set
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Capture));
                    }

                    NativeMethods.k4a_capture_set_temperature_c(this.handle, value);
                }
            }
        }

        /// <summary>
        /// Creates a new capture.
        /// </summary>
        /// <returns>A new Capture instance.</returns>
        public static Capture Create()
        {
            AzureKinectException.ThrowIfNotSuccess(
                NativeMethods.k4a_capture_create(out NativeMethods.k4a_capture_t handle));

            return new Capture(handle);
        }

        /// <summary>
        /// Creates a duplicate reference to the same Capture.
        /// </summary>
        /// <returns>A new Capture object representing the same data.</returns>
        /// <remarks>Creating a reference to the same capture does not copy the capture,
        /// or the image data stored in the capture, but creates a new managed object representing
        /// the same capture data. Each object must be independently disposed. The lifetime of the
        /// underlying capture data will be equal or greater than all of the referenced Capture objects.</remarks>
        public Capture Reference()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Capture));
                }

#pragma warning disable CA2000 // Dispose objects before losing scope
                return new Capture(this.handle.DuplicateReference());
#pragma warning restore CA2000 // Dispose objects before losing scope
            }
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            this.Dispose(true);

            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Handle the disposing of the object.
        /// </summary>
        /// <param name="disposing">true when called by Dispose(), false when called by the finalizer.</param>
        protected virtual void Dispose(bool disposing)
        {
            lock (this)
            {
                if (!this.disposedValue)
                {
                    this.cachedPrivateColor?.Dispose();
                    this.cachedPublicColor?.Dispose();
                    this.cachedPrivateDepth?.Dispose();
                    this.cachedPublicDepth?.Dispose();
                    this.cachedPrivateIR?.Dispose();
                    this.cachedPublicIR?.Dispose();

                    this.cachedPrivateColor = null;
                    this.cachedPublicColor = null;
                    this.cachedPrivateDepth = null;
                    this.cachedPublicDepth = null;
                    this.cachedPrivateIR = null;
                    this.cachedPublicIR = null;

                    this.handle.Close();
                    this.handle = null;

                    Allocator.Singleton.UnregisterForDisposal(this);
                }

                this.disposedValue = true;
            }
        }

        /// <summary>
        /// Retrieves a native image handle from the native API.
        /// </summary>
        /// <param name="nativeMethod">Native method to retrieve the image.</param>
        /// <param name="publicImage">A cached instance of the Image that we return to callers. (Callers may dispose this image).</param>
        /// <param name="privateImage">A cached instance of the Image we retain within the Capture class. (It cannot be Disposed outside of our control).</param>
        /// <remarks>
        /// If this is the first time calling the property, we construct a new wrapper.
        /// If the handle is for an Image we have already constructed a wrapper for, we return the existing wrapper.
        /// If the handle is for a different Image, or if the wrapper has already been disposed, we construct a new wrapper and dispose the old one.
        /// </remarks>
        private void GetImageWrapperAndDisposePrevious(
            Func<NativeMethods.k4a_capture_t, NativeMethods.k4a_image_t> nativeMethod,
            ref Image publicImage,
            ref Image privateImage)
        {
            // Lock must be held to ensure the Image in cache is not replaced while we are inspecting it
            // It is still possible for that Image to be accessed or Disposed while this lock is held
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Capture));
                }

                if ((publicImage != null && privateImage == null) ||
                    (publicImage == null && privateImage != null))
                {
                    throw new ArgumentException("public and private images should either both or neither be null.");
                }

                // Get the image object from the native SDK
                // We now own a reference on this image and must eventually release it.
                NativeMethods.k4a_image_t nativeImageHandle = nativeMethod(this.handle);

                // This try block ensures that we close image (and release our reference) if we fail
                // to create a new Image.
                try
                {
                    // If this Capture previously had an image
                    if (privateImage != null)
                    {
                        // Get the native pointer
                        IntPtr imageHandleValue = nativeImageHandle.DangerousGetHandle();

                        // Get the handle value from the privateImage. This image can only be disposed in the
                        // current lock, so the reference is still valid.
                        if (privateImage.DangerousGetHandle().DangerousGetHandle() != imageHandleValue)
                        {
                            // The image has changed, invalidate the current image and construct new wrappers
                            privateImage.Dispose();
                            privateImage = null;
                            publicImage.Dispose();
                            publicImage = null;
                        }
                    }

                    if (privateImage == null && !nativeImageHandle.IsInvalid)
                    {
                        // Construct a new wrapper and return it
                        // The native function may have returned
#pragma warning disable CA2000 // Dispose objects before losing scope
                        privateImage = new Image(nativeImageHandle);
                        publicImage = privateImage.Reference();
#pragma warning restore CA2000 // Dispose objects before losing scope

                        // Since we have wrapped image, it is now owned by Image and we should no longer close it
                        nativeImageHandle = null;
                    }
                }
                finally
                {
                    // Ensure the native handle is closed if we have a failure creating the Image object
                    if (nativeImageHandle != null)
                    {
                        nativeImageHandle.Close();
                    }
                }
            }
        }

        private void SetImageWrapperAndDisposePrevious(
            Action<NativeMethods.k4a_capture_t, NativeMethods.k4a_image_t> nativeMethod,
            ref Image publicImage,
            ref Image privateImage,
            Image value)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Capture));
                }

                // If the assignment is a new managed wrapper we need
                // to release the reference to the old wrapper.
                // If it is the same object though, we should not dispose it.
                if (publicImage != null && !object.ReferenceEquals(
                    publicImage, value))
                {
                    publicImage.Dispose();
                    privateImage.Dispose();
                }

                publicImage = value;
                privateImage = value?.Reference();

                nativeMethod(this.handle, privateImage.DangerousGetHandle());
            }
        }
    }
}
