/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

#ifndef K4AIMAGE_H
#define K4AIMAGE_H

// System headers
//
#include <memory>

// Library headers
//
#include <k4a/k4a.h>

// Project headers
//

namespace k4aviewer
{

template<k4a_image_format_t T> class K4AImage
{
public:
    ~K4AImage()
    {
        k4a_image_release(m_image);
    }

    K4AImage(const K4AImage &) = delete;
    K4AImage(const K4AImage &&) = delete;
    K4AImage &operator=(const K4AImage &) = delete;
    K4AImage &operator=(const K4AImage &&) = delete;

    uint8_t *GetBuffer() const
    {
        return k4a_image_get_buffer(m_image);
    }

    size_t GetSize() const
    {
        return k4a_image_get_size(m_image);
    }

    int GetWidthPixels() const
    {
        return k4a_image_get_width_pixels(m_image);
    }

    int GetHeightPixels() const
    {
        return k4a_image_get_height_pixels(m_image);
    }

    int GetStrideBytes() const
    {
        return k4a_image_get_stride_bytes(m_image);
    }

    uint64_t GetTimestampUsec() const
    {
        return k4a_image_get_timestamp_usec(m_image);
    }

private:
    friend class K4AImageFactory;
    explicit K4AImage(k4a_image_t image) : m_image(image) {}

    k4a_image_t m_image;
};

class K4AImageFactory
{
public:
    template<k4a_image_format_t T> static std::shared_ptr<K4AImage<T>> CreateK4AImage(k4a_image_t image)
    {
        if (image == nullptr)
        {
            return nullptr;
        }

        k4a_image_format_t actualFormat = k4a_image_get_format(image);
        if (actualFormat != T)
        {
            k4a_image_release(image);
            return nullptr;
        }

        return std::shared_ptr<K4AImage<T>>(new K4AImage<T>(image));
    };
};

} // namespace k4aviewer

#endif
