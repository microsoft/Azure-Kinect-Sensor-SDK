# Changes to the Kinect SDK API surface

	The following changes have been made to the Kinect SDK API surface (k4a.h, k4atypes.h) so camera manufacturers may provide 
	their own implementation of the Kinect SDK that will work with their cameras. The key change is switching from enums that identify 
	color resolution, depth mode and framerate, to structs that contain information about color mode, depth mode and fps mode. All 
	devices must support a depth sensor, but color, audio and IMU sensors are not required.

## Changes to k4atypes.h

	1. The following enums have been changed:
	
		k4a_result_t			Added K4A_RESULT_UNSUPPORTED			Returned when function requires unsupported sensor.
		k4a_buffer_result_t		Added K4A_BUFFER_RESULT_UNSUPPORTED		Returned when buffer function requires unsupported sensor.
		k4a_wait_result_t		Added K4A_WAIT_RESULT_UNSUPPORTED		Returned when wait function requires unsupported sensor.


	2. The following enums have been moved:

		k4a_color_resolution_t	Moved to include/k4ainternal/modes.h	Is no longer a part of the API surface.
		k4a_depth_mode_t		Moved to include/k4ainternal/modes.h	Is no longer a part of the API surface.
		k4a_fps_t				Moved to include/k4ainternal/modes.h	Is no longer a part of the API surface.


	3. The following enum has been added:

		k4a_device_capabilities_t	Indicates which sensors(color, depth, IMU, microphone) are supported by a device.


	4. The following structs have been added:

		k4a_device_info_t		Contains the size and version of the struct as well as vendor id, device id and 
								capabilities(color, depth, IMU, microphone).

		k4a_color_mode_info_t	Contains the size and version of the struct as well as mode id, width, height, native format, 
								horizontal/vertical fov and min/max fps.

		k4a_depth_mode_info_t	Contains the size and version of the struct as well as mode id, whether to capture passive IR only, 
								width, height, native format, horizontal/vertical fov, min/max fps and min/max range.

		k4a_fps_mode_info_t		Contains the size and version of the struct as well as mode id and fps.
	

	5. The following structs have been changed:

		k4a_calibration_t				k4a_color_resolution_t color_resolution		is now	uint32_t color_mode_id.
										k4a_depth_mode_t depth_mode					is now	uint32_t depth_mode_id.

		k4a_device_configuration_t		k4a_color_resolution_t color_resolution		is now	uint32_t color_mode_id.
										k4a_depth_mode_t depth_mode					is now	uint32_t depth_mode_id.
										k4a_fps_t camera_fps						is now	uint32_t fps_mode_id.


	4. The following #define have been added:
	
		K4A_INIT_STRUCT(T, S)	Used to safely init the new device info and color/depth/fps mode info structs.
		K4A_ABI_VERSION			Indicates the ABI version that is used in the new device info and color/depth/fps mode info structs.


	5. The following static const has been changed:

		K4A_DEVICE_CONFIG_INIT_DISABLE_ALL	The fps_mode_id is set to 0, which means off.  The color_mode_id and depth_mode_id is
											also set to 0, which means they are also both off.  To start a device, either the
											color_mode_id or the depth_mode_id must not be set to 0 and the fps_mode_id must not be 
											set to 0. To set the color_mode_id, depth_mode_id and fps_mode_id, use the new device get mode and
											modes count functions added to k4a.h.


### Changes to k4a.h

	1. The following functions have been added:

		k4a_result_t k4a_device_get_info(k4a_device_t device_handle, k4a_device_info_t *device_info)

		k4a_result_t k4a_device_get_color_mode_count(k4a_device_t device_handle, uint32_t *mode_count)
		k4a_result_t k4a_device_get_depth_mode_count(k4a_device_t device_handle, uint32_t *mode_count)
		k4a_result_t k4a_device_get_fps_mode_count(k4a_device_t device_handle, uint32_t *mode_count)
		
		k4a_result_t k4a_device_get_color_mode(k4a_device_t device_handle, uint32_t mode_index, k4a_color_mode_info_t *mode_info)
		k4a_result_t k4a_device_get_depth_mode(k4a_device_t device_handle, uint32_t mode_index, k4a_depth_mode_info_t *mode_info)
		k4a_result_t k4a_device_get_fps_mode(k4a_device_t device_handle, uint32_t mode_index, k4a_fps_mode_info_t *mode_info)

	2. The following functions have been changed:

		k4a_result_t k4a_device_get_calibration(k4a_device_t device_handle, 
												const k4a_depth_mode_t depth_mode					is now	const uint32_t depth_mode_id,
												const k4a_color_resolution_t color_resolution		is now	const uint32_t color_mode_id,
												k4a_calibration_t *calibration)
		
		k4a_result_t k4a_calibration_get_from_raw(char *raw_calibration, 
												  size_t raw_calibration_size, 
												  const k4a_depth_mode_t depth_mode					is now	const uint32_t depth_mode_id,
												  const k4a_color_resolution_t color_resolution		is now	const uint32_t color_mode_id,
												  k4a_calibration_t *calibration)

#### Using the new get device info and get color/depth/fps mode info functions added to k4a.h.

	1. Using k4a_device_get_info:

		int main(int argc, char **argv)
		{
			k4a_device_t device = NULL;
			if (K4A_RESULT_SUCCEEDED != k4a_device_open(0, &device))
			{
				printf("0: Failed to open device");
				exit(-1);
			}

		    k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, 0 };

			if (!k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
			{
				printf("Failed to get device info\n");
				exit(-1);
			}

			bool hasDepthDevice = (device_info.capabilities.bitmap.bHasDepth == 1);
			bool hasColorDevice = (device_info.capabilities.bitmap.bHasColor == 1);

			if(hasDepthDevice) 
			{
				printf("The device has a depth sensor.\n");
			}

			if(hasColorDevice) 
			{
				printf("The device has a color sensor.\n");
			}
		}


	2. Using k4a_device_get_color_mode with k4a_device_get_color_mode_count:

		int math_get_common_factor(int width, int height)
		{
			return (height == 0) ? width : math_get_common_factor(height, width % height);
		}

		int main(int argc, char **argv)
		{
			k4a_device_t device = NULL;
			if (K4A_RESULT_SUCCEEDED != k4a_device_open(0, &device))
			{
				printf("0: Failed to open device");
				exit(-1);
			}

		    k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, 0 };

			if (!k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
			{
				printf("Failed to get device info\n");
				exit(-1);
			}

			bool hasColorDevice = (device_info.capabilities.bitmap.bHasColor == 1);

			if(!hasColorDevice) 
			{
				printf("The device does not have a color sensor.\n");
				exit(-1);
			}
	
		    uint32_t color_mode_count = 0;

			if (!K4A_SUCCEEDED(k4a_device_get_color_mode_count(device, &color_mode_count)))
			{
				printf("Failed to get color mode count.\n");
				exit(-1);
			}
			
			k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, 0 };

			for (uint32_t c = 0; c < color_mode_count; c++)
			{
				if (k4a_device_get_color_mode(device, c, &color_mode_info) == K4A_RESULT_SUCCEEDED)
				{
					int width = static_cast<int>(color_mode_info.width);
					int height = static_cast<int>(color_mode_info.height);
					int common_factor = math_get_common_factor(width, height);
					
					printf("\t");
					printf("[%u] = ", c);
					printf("\t");
					if(height < 1000) 
					{
						printf(" ");
					}
					printf("%up", height);
					printf("\t");
					printf("%i:", width/common_factor);
					printf("%i", height/common_factor);
					printf("\n");
				}
			}
		}


	2. Using k4a_device_get_depth_mode with k4a_device_get_depth_mode_count:

		int main(int argc, char **argv)
		{
			k4a_device_t device = NULL;
			if (K4A_RESULT_SUCCEEDED != k4a_device_open(0, &device))
			{
				printf("0: Failed to open device");
				exit(-1);
			}

		    k4a_device_info_t device_info = { sizeof(k4a_device_info_t), K4A_ABI_VERSION, 0 };

			if (!k4a_device_get_info(device, &device_info) == K4A_RESULT_SUCCEEDED)
			{
				printf("Failed to get device info\n");
				exit(-1);
			}
			
			bool hasDepthDevice = (device_info.capabilities.bitmap.bHasDepth == 1);

			if(!hasDepthDevice) 
			{
				printf("The device does not have a depth sensor.\n");
				exit(-1);
			}
	
		    uint32_t depth_mode_count = 0;

			if (!K4A_SUCCEEDED(k4a_device_get_depth_mode_count(device, &depth_mode_count)))
			{
				printf("Failed to get depth mode count.\n");
				exit(-1);
			}
			
			k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 0 };

			for (uint32_t d = 0; d < depth_mode_count; d++)
			{
				if (k4a_device_get_depth_mode(device, d, &depth_mode_info) == K4A_RESULT_SUCCEEDED)
				{
				    int width = static_cast<int>(depth_mode_info.width);
					int height = static_cast<int>(depth_mode_info.height);
					float fov = depth_mode_info.horizontal_fov;
					
					printf("\t");
					printf("[%u] = ", d);
					printf("\t");
					if (depth_mode_info.passive_ir_only)
					{
						printf("Passive IR");
						printf("\n");
					}
					else
					{
						if (width < 1000)
						{
							printf(" ");
						}
						printf("%ix", width);
						if (height < 1000)
						{
							printf(" ");
						}
						printf("%i, ", height);
						printf("%f Deg", fov);
					}
				}
			}
		}


	2. Using k4a_device_get_fps_mode with k4a_device_get_fps_mode_count:

		int main(int argc, char **argv)
		{
			k4a_device_t device = NULL;
			if (K4A_RESULT_SUCCEEDED != k4a_device_open(0, &device))
			{
				printf("0: Failed to open device");
				exit(-1);
			}
			
		    uint32_t fps_mode_count = 0;

			if (!K4A_SUCCEEDED(k4a_device_get_fps_mode_count(device, &fps_mode_count)))
			{
				printf("Failed to get fps mode count.\n");
				exit(-1);
			}
			
			k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 0 };

			for (uint32_t f = 0; f < fps_mode_count; f++)
			{
				if (k4a_device_get_fps_mode(device, f, &fps_mode_info) == K4A_RESULT_SUCCEEDED)
				{
					printf("\t%up\n", fps_mode_info.fps);
				}
			}
		}
			