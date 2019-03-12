// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4acolorframevisualizer.h"

// System headers
//
#include <algorithm>
#include <string>

// Library headers
//
#include "libyuv.h"

// Clang parses doxygen-style comments in your source and checks for doxygen syntax errors.
// Unfortunately, some of our external dependencies have doxygen syntax errors in them, so
// we need to shut off that warning.
//
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#endif

#include "turbojpeg.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// Project headers
//
#include "assertionexception.h"
#include "k4aviewererrormanager.h"
#include "perfcounter.h"

using namespace k4aviewer;

namespace
{
ImageDimensions GetDimensionsForColorResolution(const k4a_color_resolution_t resolution)
{
    switch (resolution)
    {
    case K4A_COLOR_RESOLUTION_720P:
        return { 1280, 720 };
    case K4A_COLOR_RESOLUTION_2160P:
        return { 3840, 2160 };
    case K4A_COLOR_RESOLUTION_1440P:
        return { 2560, 1440 };
    case K4A_COLOR_RESOLUTION_1080P:
        return { 1920, 1080 };
    case K4A_COLOR_RESOLUTION_3072P:
        return { 4096, 3072 };
    case K4A_COLOR_RESOLUTION_1536P:
        return { 2048, 1536 };

    default:
        throw AssertionException("Invalid color dimensions value");
    }
}
} // namespace

class K4AColorFrameVisualizerBase
{
public:
    virtual ~K4AColorFrameVisualizerBase() = default;

protected:
    explicit K4AColorFrameVisualizerBase(k4a_color_resolution_t colorResolution) :
        m_dimensions(GetDimensionsForColorResolution(colorResolution))
    {
        m_expectedBufferSize = sizeof(RgbaPixel) * static_cast<size_t>(m_dimensions.Width * m_dimensions.Height);
    }

    size_t m_expectedBufferSize;
    ImageDimensions m_dimensions;
};

class K4AYUY2FrameVisualizer : public K4AColorFrameVisualizerBase,
                               public IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_YUY2>
{
public:
    GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> &texture) override
    {
        // libyuv does not have a function that directly converts from YUY2 to RGBA,
        // so we either have to have libyuv convert from YUY2 -> BGRA and then again
        // from BGRA -> ARGB, or we have to tell OpenGL to do the conversion as part
        // of texture upload.  Either way, we incur a performance hit by doing this
        // extra conversion.
        //
        // It looks like OpenGL's conversion is slightly faster than libyuv's, so
        // we're using that.
        //
        return K4AViewerImage::Create(&texture, nullptr, m_dimensions, GL_BGRA);
    }

    void InitializeBuffer(K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_YUY2> &buffer) override
    {
        buffer.Data.resize(m_expectedBufferSize);
    }

    ImageVisualizationResult ConvertImage(const k4a::image &image,
                                          K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_YUY2> &buffer) override
    {
        // YUY2 is a 4:2:2 format, so there are 4 bytes per 'chunk' of data, and each 'chunk' represents 2 pixels.
        //
        const int stride = m_dimensions.Width * 4 / 2;

        const auto expectedBufferSize = static_cast<size_t>(stride * m_dimensions.Height);

        if (image.get_size() != expectedBufferSize)
        {
            return ImageVisualizationResult::InvalidBufferSizeError;
        }

        static PerfCounter decode("YUY2 decode");
        PerfSample decodeSample(&decode);
        int result = libyuv::YUY2ToARGB(image.get_buffer(),                                       // src_yuy2,
                                        stride,                                                   // src_stride_yuy2,
                                        &buffer.Data[0],                                          // dst_argb,
                                        m_dimensions.Width * static_cast<int>(sizeof(RgbaPixel)), // dst_stride_argb,
                                        m_dimensions.Width,                                       // width,
                                        m_dimensions.Height                                       // height
        );
        decodeSample.End();

        if (result != 0)
        {
            return ImageVisualizationResult::InvalidImageDataError;
        }

        buffer.SourceImage = image;
        return ImageVisualizationResult::Success;
    }

    ImageVisualizationResult UpdateTexture(const K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_YUY2> &buffer,
                                           K4AViewerImage &texture) override
    {
        static PerfCounter upload("YUY2 upload");
        PerfSample uploadSample(&upload);
        return GLEnumToImageVisualizationResult(texture.UpdateTexture(&buffer.Data[0]));
    }

    K4AYUY2FrameVisualizer(k4a_color_resolution_t resolution) : K4AColorFrameVisualizerBase(resolution) {}
};

class K4ANV12FrameVisualizer : public K4AColorFrameVisualizerBase,
                               public IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_NV12>
{
public:
    GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> &texture) override
    {
        return K4AViewerImage::Create(&texture, nullptr, m_dimensions, GL_RGBA);
    }

    void InitializeBuffer(K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_NV12> &buffer) override
    {
        buffer.Data.resize(m_expectedBufferSize);
    }

    ImageVisualizationResult ConvertImage(const k4a::image &image,
                                          K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_NV12> &buffer) override
    {
        const int luminanceStride = m_dimensions.Width;
        const int hueSatStride = m_dimensions.Width;
        const uint8_t *hueSatStart = image.get_buffer() + luminanceStride * m_dimensions.Height;

        // NV12 is a 4:2:0 format, so there are half as many hue/sat pixels as luminance pixels
        //
        const auto expectedBufferSize = static_cast<size_t>(m_dimensions.Height * (luminanceStride + hueSatStride / 2));

        if (image.get_size() != expectedBufferSize)
        {
            return ImageVisualizationResult::InvalidBufferSizeError;
        }

        // libyuv refers to pixel order in system-endian order but OpenGL refers to
        // pixel order in big-endian order, which is why we create the OpenGL texture
        // as "RGBA" but then use the "ABGR" libyuv function here.
        //
        static PerfCounter decode("NV12 decode");
        PerfSample decodeSample(&decode);
        int result = libyuv::NV12ToABGR(image.get_buffer(),                                       // src_y
                                        luminanceStride,                                          // src_stride_y
                                        hueSatStart,                                              // src_vu
                                        hueSatStride,                                             // src_stride_vu
                                        &buffer.Data[0],                                          // dst_argb
                                        m_dimensions.Width * static_cast<int>(sizeof(RgbaPixel)), // dst_stride_argb
                                        m_dimensions.Width,                                       // width
                                        m_dimensions.Height                                       // height
        );
        decodeSample.End();

        if (result != 0)
        {
            return ImageVisualizationResult::InvalidImageDataError;
        }

        buffer.SourceImage = image;
        return ImageVisualizationResult::Success;
    }

    ImageVisualizationResult UpdateTexture(const K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_NV12> &buffer,
                                           K4AViewerImage &texture) override
    {
        static PerfCounter upload("NV12 upload");
        PerfSample uploadSample(&upload);
        return GLEnumToImageVisualizationResult(texture.UpdateTexture(&buffer.Data[0]));
    }

    K4ANV12FrameVisualizer(k4a_color_resolution_t resolution) : K4AColorFrameVisualizerBase(resolution) {}
};

class K4ABGRA32FrameVisualizer : public K4AColorFrameVisualizerBase,
                                 public IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_BGRA32>
{
public:
    GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> &texture) override
    {
        return K4AViewerImage::Create(&texture, nullptr, m_dimensions, GL_BGRA);
    }

    void InitializeBuffer(K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_BGRA32> &buffer) override
    {
        buffer.Data.resize(m_expectedBufferSize);
    }

    ImageVisualizationResult ConvertImage(const k4a::image &image,
                                          K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_BGRA32> &buffer) override
    {
        if (image.get_size() != static_cast<size_t>(m_dimensions.Height * m_dimensions.Width) * sizeof(RgbaPixel))
        {
            return ImageVisualizationResult::InvalidBufferSizeError;
        }

        std::copy(image.get_buffer(), image.get_buffer() + image.get_size(), buffer.Data.begin());
        buffer.SourceImage = image;
        return ImageVisualizationResult::Success;
    }

    ImageVisualizationResult UpdateTexture(const K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_BGRA32> &buffer,
                                           K4AViewerImage &texture) override
    {
        static PerfCounter upload("BGRA32 upload");
        PerfSample uploadSample(&upload);
        return GLEnumToImageVisualizationResult(texture.UpdateTexture(&buffer.Data[0]));
    }

    K4ABGRA32FrameVisualizer(k4a_color_resolution_t resolution) : K4AColorFrameVisualizerBase(resolution) {}
};

class K4AMJPGFrameVisualizer : public K4AColorFrameVisualizerBase,
                               public IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_MJPG>
{
public:
    GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> &texture) override
    {
        return K4AViewerImage::Create(&texture, nullptr, m_dimensions, GL_RGBA);
    }

    void InitializeBuffer(K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_MJPG> &buffer) override
    {
        buffer.Data.resize(m_expectedBufferSize);
    }

    ImageVisualizationResult ConvertImage(const k4a::image &image,
                                          K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_MJPG> &buffer) override
    {
        static PerfCounter mjpgDecode("MJPG decode");
        PerfSample decodeSample(&mjpgDecode);

        const int decompressStatus = tjDecompress2(m_decompressor,
                                                   image.get_buffer(),
                                                   static_cast<unsigned long>(image.get_size()),
                                                   &buffer.Data[0],
                                                   m_dimensions.Width,
                                                   0, // pitch
                                                   m_dimensions.Height,
                                                   TJPF_RGBA,
                                                   TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE);

        if (decompressStatus != 0)
        {
            return ImageVisualizationResult::InvalidImageDataError;
        }

        buffer.SourceImage = image;
        return ImageVisualizationResult::Success;
    }

    ImageVisualizationResult UpdateTexture(const K4ATextureBuffer<K4A_IMAGE_FORMAT_COLOR_MJPG> &buffer,
                                           K4AViewerImage &texture) override
    {
        static PerfCounter mjpgUpload("MJPG upload");
        PerfSample uploadSample(&mjpgUpload);
        return GLEnumToImageVisualizationResult(texture.UpdateTexture(&buffer.Data[0]));
    }

    K4AMJPGFrameVisualizer(k4a_color_resolution_t resolution) :
        K4AColorFrameVisualizerBase(resolution),
        m_decompressor(tjInitDecompress())
    {
    }

    ~K4AMJPGFrameVisualizer() override
    {
        (void)tjDestroy(m_decompressor);
    }

private:
    tjhandle m_decompressor;
};

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_YUY2>>
K4AColorFrameVisualizerFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4AYUY2FrameVisualizer>(resolution);
}

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_NV12>>
K4AColorFrameVisualizerFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4ANV12FrameVisualizer>(resolution);
}

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_BGRA32>>
K4AColorFrameVisualizerFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4ABGRA32FrameVisualizer>(resolution);
}

template<>
std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_COLOR_MJPG>>
K4AColorFrameVisualizerFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4AMJPGFrameVisualizer>(resolution);
}