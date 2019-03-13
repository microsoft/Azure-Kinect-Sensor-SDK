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
#include <k4a/k4a.h>

// Project headers
//
#include "assertionexception.h"
#include "ik4aobserver.h"
#include "ik4aframevisualizer.h"
#include "k4aimageextractor.h"
#include "k4aframeratetracker.h"
#include "k4aringbuffer.h"

namespace k4aviewer
{

template<k4a_image_format_t ImageFormat> class K4AConvertingFrameSourceImpl : public IK4ACaptureObserver
{
public:
    inline ImageVisualizationResult GetNextFrame(K4AViewerImage &textureToUpdate, k4a::image &sourceImage)
    {
        if (IsFailed())
        {
            return m_failureCode;
        }
        if (!HasData())
        {
            return ImageVisualizationResult::NoDataError;
        }

        ImageVisualizationResult result = m_frameVisualizer->UpdateTexture(*m_textureBuffers.CurrentItem(),
                                                                           textureToUpdate);
        sourceImage = m_textureBuffers.CurrentItem()->SourceImage;
        m_textureBuffers.AdvanceRead();
        return result;
    }

    inline GLenum InitializeTexture(std::shared_ptr<K4AViewerImage> &texture)
    {
        return m_frameVisualizer->InitializeTexture(texture);
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

    ~K4AConvertingFrameSourceImpl() override
    {
        m_workerThreadShouldExit = true;
        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
    };

    K4AConvertingFrameSourceImpl(std::unique_ptr<IK4AFrameVisualizer<ImageFormat>> &&frameVisualizer) :
        m_frameVisualizer(std::move(frameVisualizer))
    {
        m_textureBuffers.Initialize(
            [this](K4ATextureBuffer<ImageFormat> *buffer) { this->m_frameVisualizer->InitializeBuffer(*buffer); });

        m_workerThread = std::thread(&K4AConvertingFrameSourceImpl::WorkerThread, this);
    }

    K4AConvertingFrameSourceImpl(K4AConvertingFrameSourceImpl &) = delete;
    K4AConvertingFrameSourceImpl(K4AConvertingFrameSourceImpl &&) = delete;
    K4AConvertingFrameSourceImpl operator=(K4AConvertingFrameSourceImpl &) = delete;
    K4AConvertingFrameSourceImpl operator=(K4AConvertingFrameSourceImpl &&) = delete;

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
                // Worker thread is backed up. drop the frame
                //
                return;
            }

            *m_inputImageBuffer.InsertionItem() = std::move(image);
            m_inputImageBuffer.EndInsert();
        }
    }

private:
    static void WorkerThread(K4AConvertingFrameSourceImpl *fs)
    {
        while (!fs->m_workerThreadShouldExit)
        {
            std::unique_lock<std::mutex> lock(fs->m_mutex);
            if (!fs->m_inputImageBuffer.Empty())
            {
                // Take the image from the frame source
                //
                k4a::image imageToConvert = std::move(*fs->m_inputImageBuffer.CurrentItem());
                fs->m_inputImageBuffer.AdvanceRead();
                lock.unlock();

                if (!fs->m_textureBuffers.BeginInsert())
                {
                    // Our buffer has overflowed.  Drop the frame.
                    //
                    continue;
                }

                ImageVisualizationResult result =
                    fs->m_frameVisualizer->ConvertImage(imageToConvert, *fs->m_textureBuffers.InsertionItem());

                if (result != ImageVisualizationResult::Success)
                {
                    // We treat visualization failures as fatal.  Stop the thread.
                    //
                    fs->m_failureCode = result;
                    fs->NotifyTermination();
                    fs->m_textureBuffers.AbortInsert();
                    return;
                }

                fs->m_textureBuffers.EndInsert();
                fs->m_framerateTracker.NotifyFrame();
            }
        }
    }

    ImageVisualizationResult m_failureCode = ImageVisualizationResult::Success;
    bool m_failed = false;

    std::unique_ptr<IK4AFrameVisualizer<ImageFormat>> m_frameVisualizer;

    static constexpr size_t BufferSize = 2;
    K4ARingBuffer<K4ATextureBuffer<ImageFormat>, BufferSize> m_textureBuffers;
    K4ARingBuffer<k4a::image, BufferSize> m_inputImageBuffer;

    K4AFramerateTracker m_framerateTracker;

    std::mutex m_mutex;

    std::thread m_workerThread;
    bool m_workerThreadShouldExit = false;
};

template<k4a_image_format_t ImageFormat>
class K4AConvertingFrameSource : public K4AConvertingFrameSourceImpl<ImageFormat>
{
public:
    K4AConvertingFrameSource(std::unique_ptr<IK4AFrameVisualizer<ImageFormat>> &&frameVisualizer) :
        K4AConvertingFrameSourceImpl<ImageFormat>(std::move(frameVisualizer))
    {
    }
};

// On depth captures, we also want to track the temperature
//
template<>
class K4AConvertingFrameSource<K4A_IMAGE_FORMAT_DEPTH16> : public K4AConvertingFrameSourceImpl<K4A_IMAGE_FORMAT_DEPTH16>
{
public:
    K4AConvertingFrameSource(std::unique_ptr<IK4AFrameVisualizer<K4A_IMAGE_FORMAT_DEPTH16>> &&frameVisualizer) :
        K4AConvertingFrameSourceImpl<K4A_IMAGE_FORMAT_DEPTH16>(std::move(frameVisualizer))
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
