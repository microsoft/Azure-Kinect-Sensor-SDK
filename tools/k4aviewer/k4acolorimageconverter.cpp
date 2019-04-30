// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// Associated header
//
#include "k4acolorimageconverter.h"

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
#endif

#include "turbojpeg.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// Project headers
//
#include "k4astaticimageproperties.h"
#include "k4aviewererrormanager.h"
#include "perfcounter.h"

using namespace k4aviewer;

template<k4a_image_format_t ImageFormat> class K4AColorImageConverterBase : public IK4AImageConverter<ImageFormat>
{
public:
    virtual ~K4AColorImageConverterBase() override = default;

    ImageDimensions GetImageDimensions() const override
    {
        return m_dimensions;
    }

protected:
    explicit K4AColorImageConverterBase(k4a_color_resolution_t colorResolution) :
        m_dimensions(GetColorDimensions(colorResolution))
    {
        m_expectedBufferSize = sizeof(BgraPixel) * static_cast<size_t>(m_dimensions.Width * m_dimensions.Height);
    }

    bool ImagesAreCorrectlySized(const k4a::image &srcImage,
                                 const k4a::image &dstImage,
                                 const size_t *srcImageExpectedSize)
    {
        if (srcImageExpectedSize)
        {
            if (srcImage.get_size() != *srcImageExpectedSize)
            {
                return false;
            }
        }

        if (srcImage.get_width_pixels() != dstImage.get_width_pixels())
        {
            return false;
        }

        if (srcImage.get_height_pixels() != dstImage.get_height_pixels())
        {
            return false;
        }

        return true;
    }

    size_t m_expectedBufferSize;
    ImageDimensions m_dimensions;
};

class K4AYUY2ImageConverter : public K4AColorImageConverterBase<K4A_IMAGE_FORMAT_COLOR_YUY2>
{
public:
    ImageConversionResult ConvertImage(const k4a::image &srcImage, k4a::image *bgraImage) override
    {
        // YUY2 is a 4:2:2 format, so there are 4 bytes per 'chunk' of data, and each 'chunk' represents 2 pixels.
        //
        const int stride = m_dimensions.Width * 4 / 2;
        const size_t expectedBufferSize = static_cast<size_t>(stride * m_dimensions.Height);

        if (!ImagesAreCorrectlySized(srcImage, *bgraImage, &expectedBufferSize))
        {
            return ImageConversionResult::InvalidBufferSizeError;
        }

        static PerfCounter decode("YUY2 decode");
        PerfSample decodeSample(&decode);
        int result = libyuv::YUY2ToARGB(srcImage.get_buffer(),                                    // src_yuy2,
                                        stride,                                                   // src_stride_yuy2,
                                        bgraImage->get_buffer(),                                  // dst_argb,
                                        m_dimensions.Width * static_cast<int>(sizeof(BgraPixel)), // dst_stride_argb,
                                        m_dimensions.Width,                                       // width,
                                        m_dimensions.Height                                       // height
        );
        decodeSample.End();

        if (result != 0)
        {
            return ImageConversionResult::InvalidImageDataError;
        }

        return ImageConversionResult::Success;
    }

    K4AYUY2ImageConverter(k4a_color_resolution_t resolution) : K4AColorImageConverterBase(resolution) {}
};

class K4ANV12ImageConverter : public K4AColorImageConverterBase<K4A_IMAGE_FORMAT_COLOR_NV12>
{
public:
    ImageConversionResult ConvertImage(const k4a::image &srcImage, k4a::image *bgraImage) override
    {
        const int luminanceStride = m_dimensions.Width;
        const int hueSatStride = m_dimensions.Width;
        const uint8_t *hueSatStart = srcImage.get_buffer() + luminanceStride * m_dimensions.Height;

        // NV12 is a 4:2:0 format, so there are half as many hue/sat pixels as luminance pixels
        //
        const auto expectedBufferSize = static_cast<size_t>(m_dimensions.Height * (luminanceStride + hueSatStride / 2));

        if (!ImagesAreCorrectlySized(srcImage, *bgraImage, &expectedBufferSize))
        {
            return ImageConversionResult::InvalidBufferSizeError;
        }

        // libyuv refers to pixel order in system-endian order but OpenGL refers to
        // pixel order in big-endian order, which is why we create the OpenGL texture
        // as "RGBA" but then use the "ABGR" libyuv function here.
        //
        static PerfCounter decode("NV12 decode");
        PerfSample decodeSample(&decode);
        int result = libyuv::NV12ToARGB(srcImage.get_buffer(),                                    // src_y
                                        luminanceStride,                                          // src_stride_y
                                        hueSatStart,                                              // src_vu
                                        hueSatStride,                                             // src_stride_vu
                                        bgraImage->get_buffer(),                                  // dst_argb
                                        m_dimensions.Width * static_cast<int>(sizeof(BgraPixel)), // dst_stride_argb
                                        m_dimensions.Width,                                       // width
                                        m_dimensions.Height                                       // height
        );

        if (result != 0)
        {
            return ImageConversionResult::InvalidImageDataError;
        }

        decodeSample.End();

        return ImageConversionResult::Success;
    }

    K4ANV12ImageConverter(k4a_color_resolution_t resolution) : K4AColorImageConverterBase(resolution) {}
};

class K4ABGRA32ImageConverter : public K4AColorImageConverterBase<K4A_IMAGE_FORMAT_COLOR_BGRA32>
{
public:
    ImageConversionResult ConvertImage(const k4a::image &srcImage, k4a::image *bgraImage) override
    {
        const size_t expectedBufferSize = static_cast<size_t>(m_dimensions.Height * m_dimensions.Width) *
                                          sizeof(BgraPixel);
        if (!ImagesAreCorrectlySized(srcImage, *bgraImage, &expectedBufferSize))
        {
            return ImageConversionResult::InvalidBufferSizeError;
        }

        std::copy(srcImage.get_buffer(), srcImage.get_buffer() + srcImage.get_size(), bgraImage->get_buffer());
        return ImageConversionResult::Success;
    }

    K4ABGRA32ImageConverter(k4a_color_resolution_t resolution) : K4AColorImageConverterBase(resolution) {}
};

class K4AMJPGImageConverter : public K4AColorImageConverterBase<K4A_IMAGE_FORMAT_COLOR_MJPG>
{
public:
    ImageConversionResult ConvertImage(const k4a::image &srcImage, k4a::image *bgraImage) override
    {
        // MJPG images are not of a consistent size, so we can't compute an expected size
        //
        if (!ImagesAreCorrectlySized(srcImage, *bgraImage, nullptr))
        {
            return ImageConversionResult::InvalidBufferSizeError;
        }

        static PerfCounter mjpgDecode("MJPG decode");
        PerfSample decodeSample(&mjpgDecode);

        const int decompressStatus = tjDecompress2(m_decompressor,
                                                   srcImage.get_buffer(),
                                                   static_cast<unsigned long>(srcImage.get_size()),
                                                   bgraImage->get_buffer(),
                                                   m_dimensions.Width,
                                                   0, // pitch
                                                   m_dimensions.Height,
                                                   TJPF_BGRA,
                                                   TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE);

        if (decompressStatus != 0)
        {
            return ImageConversionResult::InvalidImageDataError;
        }

        return ImageConversionResult::Success;
    }

    K4AMJPGImageConverter(k4a_color_resolution_t resolution) :
        K4AColorImageConverterBase(resolution),
        m_decompressor(tjInitDecompress())
    {
    }

    ~K4AMJPGImageConverter() override
    {
        (void)tjDestroy(m_decompressor);
    }

private:
    tjhandle m_decompressor;
};

template<>
std::unique_ptr<IK4AImageConverter<K4A_IMAGE_FORMAT_COLOR_YUY2>>
K4AColorImageConverterFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4AYUY2ImageConverter>(resolution);
}

template<>
std::unique_ptr<IK4AImageConverter<K4A_IMAGE_FORMAT_COLOR_NV12>>
K4AColorImageConverterFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4ANV12ImageConverter>(resolution);
}

template<>
std::unique_ptr<IK4AImageConverter<K4A_IMAGE_FORMAT_COLOR_BGRA32>>
K4AColorImageConverterFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4ABGRA32ImageConverter>(resolution);
}

template<>
std::unique_ptr<IK4AImageConverter<K4A_IMAGE_FORMAT_COLOR_MJPG>>
K4AColorImageConverterFactory::Create(k4a_color_resolution_t resolution)
{
    return std14::make_unique<K4AMJPGImageConverter>(resolution);
}
