# Azure Kinect - Depth Evaluation Tools Examples

## Description

	Depth Evaluation Example Tools for K4A.

## Usage
    
	See specific usage info for each example in the example README file.
	
## Dependencies 

	These example tools require OpenCV and OpenCV Contrib to be installed. 

	To build opencv and opencv_contrib from source follow these steps

	[Ref1] General Instalation Toutorial: https://docs.opencv.org/4.5.0/d0/d3d/tutorial_general_install.html
	[Ref2] OpenCV configuration options:  https://docs.opencv.org/master/db/d05/tutorial_config_reference.html

	0. in start menu write and run "x64 Native Tools Command Prompt for VS 2019" 

	1. clone opencv and opencv_contrib
	c:\> git clone https://github.com/opencv/opencv && git -C opencv checkout 4.5.0

	c:\> git clone https://github.com/opencv/opencv_contrib && git -C opencv_contrib checkout 4.5.0

	2. build Release
	c:\> cd opencv && mkdir build && cd build 
	c:\opencv\build> cmake .. -GNinja -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules -DBUILD_opencv_world=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_PERF_TESTS:BOOL=OFF -DBUILD_TESTS:BOOL=OFF -DCMAKE_INSTALL_PREFIX=c:/opencv/build

	3. install Release
	c:\opencv\build> cd ..
	c:\opencv> cmake --build c:/opencv/build --target install

	4. build Debug
	c:>mkdir build_debug && cd build_debug 
	c:\opencv\build_debug>cmake .. -GNinja -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules -DBUILD_opencv_world=ON -DCMAKE_BUILD_TYPE=Debug -DBUILD_PERF_TESTS:BOOL=OFF -DBUILD_TESTS:BOOL=OFF -DCMAKE_INSTALL_PREFIX=c:/opencv/build

	4. install Debug
	c:\opencv\build_debug> cd ..
	c:\opencv> cmake --build c:/opencv/build_debug --target install


	*note 
	The default install location for opencv is c:\opencv\build\install\...
	However the Azure-Kinect-Sensor-SDK expects an install at c:\opencv\build\...
	to change the default install location add  
	-DCMAKE_INSTALL_PREFIX=<path_of_the_new_location>
	to the cmake .. -GNinja command 

