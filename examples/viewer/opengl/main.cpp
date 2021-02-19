// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <iostream>
#include <vector>

#include "k4adepthpixelcolorizer.h"
#include "k4apixel.h"
#include <k4a/k4a.hpp>
#include "texture.h"
#include "viewerwindow.h"

using namespace std;
using namespace viewer;

void ColorizeDepthImage(const k4a::image &depthImage,
                        DepthPixelVisualizationFunction visualizationFn,
                        std::pair<uint16_t, uint16_t> expectedValueRange,
                        std::vector<BgraPixel> *buffer);

static k4a_result_t get_device_mode_ids(k4a::device *device,
                                        k4a_color_mode_info_t *color_mode_info,
                                        k4a_depth_mode_info_t *depth_mode_info,
                                        k4a_fps_mode_info_t *fps_mode_info)
{

    // 1. get available modes from device info
    k4a_device_info_t device_info = device->get_info();
    bool hasDepthDevice = (device_info.capabilities.bitmap.bHasDepth == 1);
    bool hasColorDevice = (device_info.capabilities.bitmap.bHasColor == 1);

    // 3. get the device modes
    vector<k4a_color_mode_info_t> color_modes = device->get_color_modes();
    vector<k4a_depth_mode_info_t> depth_modes = device->get_depth_modes();
    vector<k4a_fps_mode_info_t> fps_modes = device->get_fps_modes();

    // 4. get the size of modes
    uint32_t color_mode_size = (uint32_t)color_modes.size();
    uint32_t depth_mode_size = (uint32_t)depth_modes.size();
    uint32_t fps_mode_size = (uint32_t)fps_modes.size();

    // 5. find the mode ids you want
    if (hasColorDevice && color_mode_size > 1)
    {
        for (uint32_t c = 1; c < color_mode_size; c++)
        {
            if (color_modes[c].height >= 720)
            {
                *color_mode_info = color_modes[c];
                break;
            }
        }
    }

    if (hasDepthDevice && depth_mode_size > 1)
    {
        for (uint32_t d = 1; d < depth_mode_size; d++)
        {
            if (depth_modes[d].height <= 512 && depth_modes[d].vertical_fov >= 120)
            {
                *depth_mode_info = depth_modes[d];
                break;
            }
        }
    }

    if (fps_mode_size > 1)
    {
        uint32_t max_fps = 0;
        uint32_t fps_mode_id = 0;
        for (uint32_t f = 1; f < fps_mode_size; f++)
        {
            if (fps_modes[f].fps >= (int)max_fps)
            {
                max_fps = (uint32_t)fps_modes[f].fps;
                fps_mode_id = f;
            }
        }
        *fps_mode_info = fps_modes[fps_mode_id];
    }

    // 6. fps mode id must not be set to 0, which is Off, and either color mode id or depth mode id must not be set
    // to 0
    if (fps_mode_info->mode_id == 0)
    {
        cout << "Fps mode id must not be set to 0 (Off)" << endl;
        exit(-1);
    }

    if (color_mode_info->mode_id == 0 && depth_mode_info->mode_id == 0)
    {
        cout << "Either color mode id or depth mode id must not be set to 0 (Off)" << endl;
        exit(-1);
    }

    return K4A_RESULT_SUCCEEDED;
}

int main()
{
    try
    {
        // Check for devices
        //
        const uint32_t deviceCount = k4a::device::get_installed_count();
        if (deviceCount == 0)
        {
            throw std::runtime_error("No Azure Kinect devices detected!");
        }

        std::cout << "Started opening K4A device..." << std::endl;

        k4a::device dev = k4a::device::open(K4A_DEVICE_DEFAULT);

        k4a_color_mode_info_t color_mode_info = { sizeof(k4a_color_mode_info_t), K4A_ABI_VERSION, 0 };
        k4a_depth_mode_info_t depth_mode_info = { sizeof(k4a_depth_mode_info_t), K4A_ABI_VERSION, 0 };
        k4a_fps_mode_info_t fps_mode_info = { sizeof(k4a_fps_mode_info_t), K4A_ABI_VERSION, 0 };

        if (!K4A_SUCCEEDED(get_device_mode_ids(&dev, &color_mode_info, &depth_mode_info, &fps_mode_info)))
        {
            cout << "Failed to get device mode ids" << endl;
            exit(-1);
        }

        // Start the device
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        config.color_mode_id = color_mode_info.mode_id;
        config.depth_mode_id = depth_mode_info.mode_id;
        config.fps_mode_id = fps_mode_info.mode_id;

        // This means that we'll only get captures that have both color and
        // depth images, so we don't need to check if the capture contains
        // a particular type of image.
        //
        config.synchronized_images_only = true;

        dev.start_cameras(&config);

        std::cout << "Finished opening K4A device." << std::endl;

        // Create the viewer window.
        //
        ViewerWindow &window = ViewerWindow::Instance();
        window.Initialize("Simple Azure Kinect Viewer", 1440, 900);

        // Textures we can give to OpenGL / the viewer window to render.
        Texture depthTexture = window.CreateTexture({ depth_mode_info.width, depth_mode_info.height });
        Texture colorTexture = window.CreateTexture({ color_mode_info.width, color_mode_info.height });

        // A buffer containing a BGRA color representation of the depth image.
        // This is what we'll end up giving to depthTexture as an image source.
        // We don't need a similar buffer for the color image because the color
        // image already comes to us in BGRA32 format.
        //
        std::vector<BgraPixel> depthTextureBuffer;

        // viewer.BeginFrame() will start returning false when the user closes the window.
        //
        while (window.BeginFrame())
        {
            // Poll the device for new image data.
            //
            // We set the timeout to 0 so we don't block if there isn't an available frame.
            //
            // This works here because we're doing the work on the same thread that we're
            // using for the UI, and the ViewerWindow class caps the framerate at the
            // display's refresh rate (the EndFrame() call blocks on the display's sync).
            //
            // If we don't have new image data, we'll just reuse the textures we generated
            // from the last time we got a capture.
            //
            k4a::capture capture;
            if (dev.get_capture(&capture, std::chrono::milliseconds(0)))
            {
                const k4a::image depthImage = capture.get_depth_image();
                const k4a::image colorImage = capture.get_color_image();

                // If we hadn't set synchronized_images_only=true above, we'd need to do
                // if (depthImage) { /* stuff */ } else { /* ignore the image */ } here.
                //
                // Depth data is in the form of uint16_t's representing the distance in
                // millimeters of the pixel from the camera, so we need to convert it to
                // a BGRA image before we can upload and show it.
                //
                ColorizeDepthImage(depthImage,
                                   K4ADepthPixelColorizer::ColorizeBlueToRed,
                                   { (uint16_t)depth_mode_info.min_range, (uint16_t)depth_mode_info.max_range },
                                   &depthTextureBuffer);
                depthTexture.Update(&depthTextureBuffer[0]);

                // Since we're using BGRA32 mode, we can just upload the color image directly.
                // If you want to use one of the other modes, you have to do the conversion
                // yourself.
                //
                colorTexture.Update(reinterpret_cast<const BgraPixel *>(colorImage.get_buffer()));
            }

            // Show the windows
            //
            const ImVec2 windowSize(window.GetWidth() / 2.f, static_cast<float>(window.GetHeight()));

            window.ShowTexture("Depth", depthTexture, ImVec2(0.f, 0.f), windowSize);
            window.ShowTexture("Color", colorTexture, ImVec2(window.GetWidth() / 2.f, 0.f), windowSize);

            // This will tell ImGui/OpenGL to render the frame, and will block until the next vsync.
            //
            window.EndFrame();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << "Press [enter] to exit." << std::endl;
        std::cin.get();
        return 1;
    }
    return 0;
}

// Given a depth image, output a BGRA-formatted color image into buffer, using
// expectedValueRange to define what min/max values the depth image should have.
// Low values are blue, high values are red.
//
void ColorizeDepthImage(const k4a::image &depthImage,
                        DepthPixelVisualizationFunction visualizationFn,
                        std::pair<uint16_t, uint16_t> expectedValueRange,
                        std::vector<BgraPixel> *buffer)
{
    // This function assumes that the image is made of depth pixels (i.e. uint16_t's),
    // which is only true for IR/depth images.
    //
    const k4a_image_format_t imageFormat = depthImage.get_format();
    if (imageFormat != K4A_IMAGE_FORMAT_DEPTH16 && imageFormat != K4A_IMAGE_FORMAT_IR16)

    {
        throw std::logic_error("Attempted to colorize a non-depth image!");
    }

    const int width = depthImage.get_width_pixels();
    const int height = depthImage.get_height_pixels();

    buffer->resize(static_cast<size_t>(width * height));

    const uint16_t *depthData = reinterpret_cast<const uint16_t *>(depthImage.get_buffer());
    for (int h = 0; h < height; ++h)
    {
        for (int w = 0; w < width; ++w)
        {
            const size_t currentPixel = static_cast<size_t>(h * width + w);
            (*buffer)[currentPixel] = visualizationFn(depthData[currentPixel],
                                                      expectedValueRange.first,
                                                      expectedValueRange.second);
        }
    }
}
