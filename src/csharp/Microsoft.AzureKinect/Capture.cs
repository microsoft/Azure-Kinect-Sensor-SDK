using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    public class Capture : IDisposable
    {
        internal Capture(NativeMethods.k4a_capture_t handle)
        {
            this.handle = handle;
        }

        public static Capture Create()
        {
            NativeMethods.k4a_capture_t handle;

            Exception.ThrowIfNotSuccess(
                NativeMethods.k4a_capture_create(out handle));

            return new Capture(handle);
        }


        private Image _Color;
        private Image _Depth;
        private Image _IR;

        public Image Color
        {
            get
            {
                NativeMethods.k4a_image_t image = NativeMethods.k4a_capture_get_color_image(handle);
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    if (_Color != null)
                    {
                        if (_Color.DangerousGetHandle() == image.DangerousGetHandle())
                        {
                            // The image is the same as the current managed wrapper

                            // Close the native handle to release the newly created reference
                            image.Close();
                            image = null;

                            // Return the previously created wrapper
                            return _Color;
                        }
                        else
                        {
                            // The color image has changed, invalidate the current color image and construct
                            // a new wrapper
                            _Color.Dispose();
                            _Color = null;
                        }
                    }

                    // Construct a new wrapper and return it
                    _Color = new UnsafeImage(image);
                    return _Color;
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    // If the assignment is a new managed wrapper we need
                    // to release the reference to the old wrapper
                    if (_Color != null && !object.ReferenceEquals(
                        _Color, value))
                    {
                        _Color.Dispose();
                    }
                    _Color = value;
                }
            }
        }


        public Image Depth {
            get
            {
                NativeMethods.k4a_image_t image = NativeMethods.k4a_capture_get_depth_image(handle);
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    if (_Depth != null)
                    {
                        if (_Depth.DangerousGetHandle() == image.DangerousGetHandle())
                        {
                            // The image is the same as the current managed wrapper

                            // Close the native handle to release the newly created reference
                            image.Close();
                            image = null;

                            // Return the previously created wrapper
                            return _Depth;
                        }
                        else
                        {
                            // The color image has changed, invalidate the current color image and construct
                            // a new wrapper
                            _Depth.Dispose();
                            _Depth = null;
                        }
                    }

                    // Construct a new wrapper and return it
                    _Depth = new UnsafeImage(image);
                    return _Depth;
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    // If the assignment is a new managed wrapper we need
                    // to release the reference to the old wrapper
                    if (_Depth != null && !object.ReferenceEquals(
                        _Depth, value))
                    {
                        _Depth.Dispose();
                    }
                    _Depth = value;
                }
            }
        }

        public Image IR {
            get
            {
                NativeMethods.k4a_image_t image = NativeMethods.k4a_capture_get_ir_image(handle);
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    if (_IR != null)
                    {
                        if (_IR.DangerousGetHandle() == image.DangerousGetHandle())
                        {
                            // The image is the same as the current managed wrapper

                            // Close the native handle to release the newly created reference
                            image.Close();
                            image = null;

                            // Return the previously created wrapper
                            return _IR;
                        }
                        else
                        {
                            // The color image has changed, invalidate the current color image and construct
                            // a new wrapper
                            _IR.Dispose();
                            _IR = null;
                        }
                    }

                    // Construct a new wrapper and return it
                    _IR = new UnsafeImage(image);
                    return _IR;
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    // If the assignment is a new managed wrapper we need
                    // to release the reference to the old wrapper
                    if (_IR != null && !object.ReferenceEquals(
                        _IR, value))
                    {
                        _IR.Dispose();
                    }
                    _IR = value;
                }
            }
        }

        public float Temperature
        {
            get
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    return NativeMethods.k4a_capture_get_temperature_c(handle);
                }
            }
            set
            {
                lock (this)
                {
                    if (disposedValue)
                        throw new ObjectDisposedException(nameof(Capture));

                    NativeMethods.k4a_capture_set_temperature_c(handle, value);
                }
            }
        }

        public Capture Reference()
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Capture));

                return new Capture(handle.DuplicateReference());
            }
        }

        private NativeMethods.k4a_capture_t handle;

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            lock (this)
            {
                if (!disposedValue)
                {
                    if (disposing)
                    {
                        // TODO: dispose managed state (managed objects).
                        if (_Color != null)
                        {
                            _Color.Dispose();
                            _Color = null;
                        }
                        if (_Depth != null)
                        {
                            _Depth.Dispose();
                            _Depth = null;
                        }
                        if (_IR != null)
                        {
                            _IR.Dispose();
                            _IR = null;
                        }
                    }

                    // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                    // TODO: set large fields to null.
                    handle.Close();
                    handle = null;

                    disposedValue = true;
                }
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Capture()
        {
          // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
          Dispose(false);
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
            // TODO: uncomment the following line if the finalizer is overridden above.
             GC.SuppressFinalize(this);
        }
        #endregion

    }
}
