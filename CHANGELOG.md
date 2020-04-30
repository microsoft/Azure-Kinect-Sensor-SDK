## Change Log

### v1.4.1

* Added NEON for ARM64
* Failed conversion of MJPEG to BGRA is now a warning - not an error.

### v1.4.0

* Added ARM64 Suport.
* On Windows Opencv-4.1.1 is now being used and tested.
* CPP; Adding record.hpp, updated playback.hpp
* Fixed small error in transformation functions
* Updated K4aRecorder allow:
  * Setting manual exposure based on exposure time.
  * Record BGRA32 format.
* Added transformation API's to CSharp

### v1.3.0 

* On Windows VS dependencies are now dynamically linked and require redistributables for
[VS2015](https://www.microsoft.com/en-us/download/details.aspx?id=48145) or newer.
* Error and Warning messages have been cleaned up to be less verbose.
* Multi camera sync and capture green screen example added.
* Added new k4a_calibration_color_2d_to_depth_2d API  to transform pixel from color image to depth image with searching on epipolar line
* Added a capture::handle() method to the C++ wrapper, allowing access to the underlying k4a_capture_t when using the C++ wrapper.

### v1.2.0

* CSharp support added.
* Depth Engine breaking changes to 2.0, SDK now relies on this new version.
* Firmware updates for better USB compatibility.
* Added new API's k4a_image_get_device_timestamp_usec(), k4a_image_get_system_timestamp_usec(), 
k4a_image_set_device_timestamp_usec(), k4a_image_set_system_timestamp_usec(), and k4a_image_set_exposure_usec(). (#350)
* Deprecated API's k4a_image_get_timestamp_usec(), k4a_image_set_timestamp_usec(), and k4a_image_set_exposure_time_usec().
* Added new transformation API k4a_transformation_depth_image_to_color_camera_custom(). (#566)
* Fixed color exposure get & set API's. (#515)
* The C++ API for playback was made public. (#493)
* Added custom track recording and playback API. (#246)
* All playback API functions now return timestamps in device time instead of relative to start of recording. (#592)
* Deprecated k4a_playback_get_last_timestamp_usec() and replaced with k4a_playback_get_recording_length_usec().

### v1.1.0

* Clean up repo documentation for going public.
* New API k4a_device_get_color_control_capabilities() to read color control capabilities was added. (#319)
* C++ wrapper added.
* Linux, Color camera support added.
* K4A_COLOR_CONTROL_AUTO_EXPOSURE_PRIORITY deprecated. (#277)
* New K4A_\*_TRACK tags added to recordings for track identification. (#259)
* Playback External Sync example added. (#274)
* Playback API format conversion support added (MJPG -> BGRA32, etc...). (#237)
* Playback IMU API added. (#213)
* Various playback performance improvements, including read-ahead and cluster caching. (#203, #189, #142)
* k4a_playback_seek_timestamp() functionality changed to fix edge cases. (#141)

### v1.0.0

* Breaking Change to pre-production devices, update container ID & serial number
* Added support for multiple devices on a single PC.

### v0.8.0

* Output intrinsic calibrated IMU data
* Changed units of IMU accelerometer reading from G to meters per second squared.
* Changed units of IMU Gyroscope reading from degrees per second to radians per second.
* Breaking change to k4a_calibration_get_from_raw API, size of source calibration string is now required along with the 
source calibration.
* Added FW version minimum bar check
* Integrated GPU transform engine into SDK to accelerate transformation between depth image and color image.
* Depth Engine plugin versioning through binding with SDK during loading and more logging added.

### v0.7.1

* Added file based record and playback headers to SDK
* Swapped tangential distortion parameters p1 and p2 in intrinsic calibration to align with OpenCV.

### v0.7.0

* Removed deprecated API's and structures
* Drop depth captures if they arrive successfully over USB but are too small.
* Drop IMU captures when the timestamp is reported is not valid.
* On k4a_device_open, stop depth and IMU sensors from streaming in the event the previous session didn't clean up.
* Renamed k4a_camera_calibration_t to k4a_calibration_camera_t for naming consistency in k4atypes.h.
* Renamed k4a_intrinsic_parameters_t to k4a_calibration_intrinsic_parameters_t for naming consistency in k4atypes.h.
* OpenCV compatibility
    * Added support for Brown-Conrady lens model.
    * Modified parameters of intrinsic calibration to be pixelized and 0-centered instead of unitized and 0-cornered.

### v0.6.0

* Added support for k4a_image_t and family of API's to support access.
* Removed direction image access via k4a_capture_ API's
* Deprecated most k4a_capture_get_* API's

### v0.5.2

* Switched firmware file to manufacturing version to address bugs

### v0.5.1

* Add firmware binary blob to SDK

### v0.5.0

* destub_depth_engine_process_frame error was converted to a warning and message softened.
* Added synchronized_images_only to k4a_device_configuration_t
* USB depth transfer request size more closely matches expected image size.
* k4aviewer now can save default settings
* Bug fixes

### v0.3.0

* Additional color camera controls
* Bug fixes
* Support native RGB
* Support for external sync connections
* Point cloud viewer
* Updated K4AViewer to support High DPI displays
* Removed k4a image format K4A\_IMAGE\_FORMAT\_UNKNOWN
* Removed k4a FPS value K4A\_FRAMES\_PER\_SECOND\_OFF
* Added tool to run firmware update
* IMU recording and device selection was added to k4arecorder
* Removed x86 builds from the SDK
* Removed DepthEngine.pdb from the SDK
* Minor breaking change to k4a_device_configuration_t; color_fps & depth_fps consolidated to camera_fps

### v0.2.0

Sensor SDK v0.2.0 includes major refactoring to API

* API refactoring (**breaking change**)
* Depth-RGB correlation API
* ***Note!*** When using both Depth-RGB cameras same time they will be synchronized and can only run with the same framerate. Option to have separate frame rates have been disabled.
* Additional frame-meta data support (e.g. resolution, laser temperature)
* Coordinate space helpers (Project 3D to 2D, Unproject 2D to 3D, Extrinsic transformation (3D to 3D)
* Sensor recording API refactoring and improvements
* Sensor recorder and Kinect for Azure viewer updated to new API

### v0.1.0

This is the very first internal sensor SDK release

* Depth camera access
* RGB camera access
* RGB camera exposure control
* IMU access
* Device calibration blob access
* Frame meta-data for Depth and RGB device timestamp
* Kinect for Azure Viewer, samples (streaming, enumeration,..)
* Recording tool (Depth and RGB streams)
