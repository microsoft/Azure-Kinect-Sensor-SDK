// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef K4ACONVERTINGIMAGESOURCE_H
#define K4ACONVERTINGIMAGESOURCE_H

// System headers
//
#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

// Library headers
//
#include <k4a/k4a.hpp>

// Project headers
//
#include "ik4aobserver.h"
#include "ik4aimageconverter.h"
#include "k4aimageextractor.h"
#include "k4aframeratetracker.h"
#include "k4aringbuffer.h"

namespace k4aviewer
{

template<k4a_image_format_t ImageFormat> class K4AConvertingImageSourceImpl : public IK4ACaptureObserver
{
public:
    inline ImageConversionResult GetNextImage(K4AViewerImage *textureToUpdate, k4a::image *sourceImage)
    {
        if (IsFailed())
        {
            return m_failureCode;
        }
        if (!HasData())
        {
            return ImageConversionResult::NoDataError;
        }

        GLenum result = textureToUpdate->UpdateTexture(m_textureBuffers.CurrentItem()->Bgra.get_buffer());
        *sourceImage = m_textureBuffers.CurrentItem()->Source;

        m_textureBuffers.AdvanceRead();
        return GLEnumToImageConversionResult(result);
    }

    inline GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> *texture)
    {
        return K4AViewerImage::Create(texture, nullptr, m_imageConverter->GetImageDimensions());
    }

    double GetFrameRate() const
    {
        return m_framerateTracker.GetFramerate();
    }

    bool IsFailed() const
    {
        return m_failed;
    }

    bool HasData()
    {
        return !m_textureBuffers.Empty();
    }

    void NotifyData(const k4a::capture &data) override
    {
        NotifyDataImpl(data);
    }

    void NotifyTermination() override
    {
        m_workerThreadShouldExit = true;
        m_failed = true;
    }

    void ClearData() override
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_textureBuffers.Clear();
        m_inputImageBuffer.Clear();
    }

    ~K4AConvertingImageSourceImpl() override
    {
        m_workerThreadShouldExit = true;
        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
    };

    K4AConvertingImageSourceImpl(std::unique_ptr<IK4AImageConverter<ImageFormat>> &&imageConverter) :
        m_imageConverter(std::move(imageConverter))
    {
        ImageDimensions dimensions = m_imageConverter->GetImageDimensions();
        m_textureBuffers.Initialize([dimensions](ConvertedImagePair *bufferItem) {
            bufferItem->Bgra = k4a::image::create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                                                  dimensions.Width,
                                                  dimensions.Height,
                                                  dimensions.Width * static_cast<int>(sizeof(BgraPixel)));
        });

        m_workerThread = std::thread(&K4AConvertingImageSourceImpl::WorkerThread, this);
    }

    K4AConvertingImageSourceImpl(K4AConvertingImageSourceImpl &) = delete;
    K4AConvertingImageSourceImpl(K4AConvertingImageSourceImpl &&) = delete;
    K4AConvertingImageSourceImpl operator=(K4AConvertingImageSourceImpl &) = delete;
    K4AConvertingImageSourceImpl operator=(K4AConvertingImageSourceImpl &&) = delete;

protected:
    void NotifyDataImpl(const k4a::capture &data)
    {
        // If the capture we're being notified of doesn't contain data
        // from the mode we're listening for, we don't want to update
        // our data.
        //
        k4a::image image = K4AImageExtractor::GetImageFromCapture<ImageFormat>(data);
        if (image != nullptr)
        {
            // Hand the image off to our worker thread.
            //
            std::lock_guard<std::mutex> lock(m_mutex);

            if (!m_inputImageBuffer.BeginInsert())
            {
                // Worker thread is backed up. drop the image
                //
                return;
            }

            *m_inputImageBuffer.InsertionItem() = std::move(image);
            m_inputImageBuffer.EndInsert();
        }
    }

private:
    static void WorkerThread(K4AConvertingImageSourceImpl *fs)
    {
        while (!fs->m_workerThreadShouldExit)
        {
            std::unique_lock<std::mutex> lock(fs->m_mutex);
            if (!fs->m_inputImageBuffer.Empty())
            {
                // Take the image from the image source
                //
                k4a::image imageToConvert = std::move(*fs->m_inputImageBuffer.CurrentItem());
                fs->m_inputImageBuffer.AdvanceRead();
                lock.unlock();

                if (!fs->m_textureBuffers.BeginInsert())
                {
                    // Our buffer has overflowed.  Drop the image.
                    //
                    continue;
                }

                ImageConversionResult result =
                    fs->m_imageConverter->ConvertImage(imageToConvert, &fs->m_textureBuffers.InsertionItem()->Bgra);

                if (result != ImageConversionResult::Success)
                {
                    // We treat visualization failures as fatal.  Stop the thread.
                    //
                    fs->m_failureCode = result;
                    fs->NotifyTermination();
                    fs->m_textureBuffers.AbortInsert();
                    return;
                }

                // Save off the source image so the viewer can show things like pixel values
                //
                fs->m_textureBuffers.InsertionItem()->Source = std::move(imageToConvert);

                fs->m_textureBuffers.EndInsert();
                fs->m_framerateTracker.NotifyFrame();
            }
        }
    }

    ImageConversionResult m_failureCode = ImageConversionResult::Success;
    bool m_failed = false;

    std::unique_ptr<IK4AImageConverter<ImageFormat>> m_imageConverter;

    static constexpr size_t BufferSize = 2;

    struct ConvertedImagePair
    {
        k4a::image Source;
        k4a::image Bgra;
    };

    K4ARingBuffer<ConvertedImagePair, BufferSize> m_textureBuffers;
    K4ARingBuffer<k4a::image, BufferSize> m_inputImageBuffer;

    K4AFramerateTracker m_framerateTracker;

    std::mutex m_mutex;

    std::thread m_workerThread;
    bool m_workerThreadShouldExit = false;
};

template<k4a_image_format_t ImageFormat>
class K4AConvertingImageSource : public K4AConvertingImageSourceImpl<ImageFormat>
{
public:
    K4AConvertingImageSource(std::unique_ptr<IK4AImageConverter<ImageFormat>> &&imageConverter) :
        K4AConvertingImageSourceImpl<ImageFormat>(std::move(imageConverter))
    {
    }
};

// On depth captures, we also want to track the temperature
//
template<>
class K4AConvertingImageSource<K4A_IMAGE_FORMAT_DEPTH16> : public K4AConvertingImageSourceImpl<K4A_IMAGE_FORMAT_DEPTH16>
{
public:
    K4AConvertingImageSource(std::unique_ptr<IK4AImageConverter<K4A_IMAGE_FORMAT_DEPTH16>> &&imageConverter) :
        K4AConvertingImageSourceImpl<K4A_IMAGE_FORMAT_DEPTH16>(std::move(imageConverter))
    {
    }

    float GetLastSensorTemperature()
    {
        return m_lastSensorTemperature;
    }

    void NotifyData(const k4a::capture &data) override
    {
        m_lastSensorTemperature = data.get_temperature_c();
        NotifyDataImpl(data);
    }

private:
    float m_lastSensorTemperature = 0.0f;
};

} // namespace k4aviewer

#endif
