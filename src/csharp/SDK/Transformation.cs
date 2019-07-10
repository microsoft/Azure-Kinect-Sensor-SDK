// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class Transformation : IDisposable
    {
        readonly NativeMethods.k4a_transformation_t handle;

        readonly Calibration calibration;
        public Transformation(Calibration calibration)
        {
            this.calibration = calibration;
            this.handle = NativeMethods.k4a_transformation_create(ref calibration);
            if (this.handle == null)
            {
                throw new AzureKinectException("Failed to create transformation object");
            }
        }

        public ArrayImage<ushort> DepthImageToColorCamera(Capture capture)
        {
            return DepthImageToColorCamera(capture.Depth);
        }

        public ArrayImage<ushort> DepthImageToColorCamera(Image depth)
        {
            ArrayImage<ushort> image = new ArrayImage<ushort>(ImageFormat.Depth16,
                calibration.color_camera_calibration.resolution_width,
                calibration.color_camera_calibration.resolution_height)
            {
                Timestamp = depth.Timestamp
            };

            DepthImageToColorCamera(depth, image);

            return image;
        }

        public void DepthImageToColorCamera(Capture capture, Image transformed)
        {
            DepthImageToColorCamera(capture.Depth, transformed);
        }

        public void DepthImageToColorCamera(Image depth, Image transformed)
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Transformation));

                // Create a new reference to the Image objects so that they cannot be disposed while
                // we are performing the transformation
                using (Image depthReference = depth.Reference())
                using (Image transformedReference = transformed.Reference())
                {
                    AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_transformation_depth_image_to_color_camera(
                        handle,
                        depthReference.DangerousGetHandle(),
                        transformedReference.DangerousGetHandle()
                        ));
                }
            }
        }

        public ArrayImage<BGRA> ColorImageToDepthCamera(Capture capture)
        {
            return ColorImageToDepthCamera(capture.Depth, capture.Color);
        }

        public ArrayImage<BGRA> ColorImageToDepthCamera(Image depth, Image color)
        {
            ArrayImage<BGRA> transformed = new ArrayImage<BGRA>(ImageFormat.ColorBGRA32,
                calibration.depth_camera_calibration.resolution_width,
                calibration.depth_camera_calibration.resolution_height)
            {
                Exposure = color.Exposure,
                ISOSpeed = color.ISOSpeed,
                Timestamp = color.Timestamp,
                WhiteBalance = color.WhiteBalance
            };

            
            ColorImageToDepthCamera(depth, color, transformed);

            return transformed;
        }

        public void ColorImageToDepthCamera(Capture capture, Image transformed)
        {
            ColorImageToDepthCamera(capture.Depth, capture.Color, transformed);
        }

        public void ColorImageToDepthCamera(Image depth, Image color, Image transformed)
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Transformation));

                // Create a new reference to the Image objects so that they cannot be disposed while
                // we are performing the transformation
                using (Image depthReference = depth.Reference())
                using (Image colorReference = color.Reference())
                using (Image transformedReference = transformed.Reference())
                {
                    AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_transformation_color_image_to_depth_camera(
                        handle,
                        depthReference.DangerousGetHandle(),
                        colorReference.DangerousGetHandle(),
                        transformedReference.DangerousGetHandle()));
                }
            }
        }

        public ArrayImage<Short3> DepthImageToPointCloud(Image depth, Calibration.DeviceType camera = Calibration.DeviceType.Depth)
        {
            ArrayImage<Short3> pointCloud = new ArrayImage<Short3>(ImageFormat.Custom, 
                depth.WidthPixels, depth.HeightPixels);

            DepthImageToPointCloud(depth, pointCloud, camera);

            return pointCloud;
        }

        public void DepthImageToPointCloud(Image depth, Image pointCloud, Calibration.DeviceType camera = Calibration.DeviceType.Depth)
        {
            lock (this)
            {
                if (disposedValue)
                    throw new ObjectDisposedException(nameof(Transformation));

                // Create a new reference to the Image objects so that they cannot be disposed while
                // we are performing the transformation
                using (Image depthReference = depth.Reference())
                using (Image pointCloudReference = pointCloud.Reference())
                {
                    AzureKinectException.ThrowIfNotSuccess(NativeMethods.k4a_transformation_depth_image_to_point_cloud(
                        handle,
                        depthReference.DangerousGetHandle(),
                        camera,
                        pointCloudReference.DangerousGetHandle()));
                }
            }
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    // TODO: dispose managed state (managed objects).
                }

                // TODO: free unmanaged resources (unmanaged objects) and override a finalizer below.
                // TODO: set large fields to null.
                handle.Close();

                disposedValue = true;
            }
        }

        // TODO: override a finalizer only if Dispose(bool disposing) above has code to free unmanaged resources.
        ~Transformation()
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
