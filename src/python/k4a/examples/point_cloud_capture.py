import numpy as np
import k4a

if __name__ == '__main__':
    # Open Device
    device = k4a.Device.open()

    # Start Cameras
    device_config = k4a.DEVICE_CONFIG_BGRA32_1080P_NFOV_UNBINNED_FPS15
    device.start_cameras(device_config)

    # Get Calibration
    calibration = device.get_calibration(
        depth_mode=device_config.depth_mode,
        color_resolution=device_config.color_resolution)

    # Create Transformation
    transformation = k4a.Transformation(calibration)

    # Capture One Frame
    capture = device.get_capture(-1)

    # Get Point Cloud
    xyz_image = transformation.depth_image_to_point_cloud(capture.depth, k4a.ECalibrationType.DEPTH)

    # Save Point Cloud To Ascii Format File. Interleave the [X, Y, Z] channels into [x0, y0, z0, x1, y1, z1, ...]
    width, height, channels = xyz_image.data.shape
    xyz_data = np.empty((width * height, channels,), dtype=xyz_image.data.dtype)
    xyz_data[:,0] = xyz_image.data[:,:,0].reshape(width*height)
    xyz_data[:,1] = xyz_image.data[:,:,1].reshape(width*height)
    xyz_data[:,2] = xyz_image.data[:,:,2].reshape(width*height)
    
    np.savetxt('data.txt', xyz_data, delimiter=' ', fmt='%u') # save ascii format (x y z\n x y z\n x y z\n ...)

    # Stop Cameras
    device.stop_cameras()