/** \file playback.hpp
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK - C++ wrapper.
 */

#ifndef K4A_PLAYBACK_HPP
#define K4A_PLAYBACK_HPP

#include <k4a/k4a.hpp>
#include <k4arecord/playback.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace k4a
{

/** \class playback playback.hpp
 * Wrapper for \ref k4a_playback_t
 *
 * Wraps a handle for a playback object
 *
 * \sa k4a_playback_t
 */
class playback
{
public:
    /** Creates a k4a::playback from a k4a_playback_t
     * Takes ownership of the handle, i.e. you should not call
     * k4a_playback_close on the handle after giving it to the
     * k4a::playback; the k4a::playback will take care of that.
     */
    playback(k4a_playback_t handle = nullptr) noexcept : m_handle(handle) {}

    /** Moves another k4a::playback into a new k4a::playback
     */
    playback(playback &&other) noexcept : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    playback(const playback &) = delete;

    ~playback()
    {
        close();
    }

    playback &operator=(const playback &) = delete;

    /** Moves another k4a::playback into this k4a::playback; other is set to invalid
     */
    playback &operator=(playback &&other) noexcept
    {
        if (this != &other)
        {
            close();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }

        return *this;
    }

    /** Returns true if the k4a::playback is valid, false otherwise
     */
    operator bool() const noexcept
    {
        return m_handle != nullptr;
    }

    /** Closes a K4A recording.
     *
     * \sa k4a_playback_close
     */
    void close() noexcept
    {
        if (m_handle != nullptr)
        {
            k4a_playback_close(m_handle);
            m_handle = nullptr;
        }
    }

    /** Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_playback_get_raw_calibration
     */
    std::vector<uint8_t> get_raw_calibration() const
    {
        std::vector<uint8_t> calibration;
        size_t buffer = 0;
        k4a_buffer_result_t result = k4a_playback_get_raw_calibration(m_handle, &calibration[0], &buffer);

        if (result == K4A_BUFFER_RESULT_TOO_SMALL && buffer > 1)
        {
            calibration.resize(buffer);
            result = k4a_playback_get_raw_calibration(m_handle, &calibration[0], &buffer);
        }

        if (result != K4A_BUFFER_RESULT_SUCCEEDED)
        {
            throw error("Failed to read raw device calibration from recording!");
        }

        return calibration;
    }

    /** Get the camera calibration for the K4A device that made the recording, which is used for all transformation
     * functions. Throws error on failure.
     *
     * \sa k4a_playback_get_calibration
     */
    calibration get_calibration() const
    {
        calibration calib;
        k4a_result_t result = k4a_playback_get_calibration(m_handle, &calib);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to read device calibration from recording!");
        }

        return calib;
    }

    /** Gets the configuration of the recording
     *
     * \sa k4a_playback_get_record_configuration
     */
    k4a_record_configuration_t get_record_configuration() const
    {
        k4a_record_configuration_t config;
        k4a_result_t result = k4a_playback_get_record_configuration(m_handle, &config);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to read record configuration!");
        }

        return config;
    }

    /** Get the next capture in the recording
     * Returns true if a capture was available, false if there are none left
     * Throws error on failure.
     *
     * \sa k4a_playback_get_next_capture
     */
    bool get_next_capture(capture *cap)
    {
        k4a_capture_t capture_handle;
        k4a_stream_result_t result = k4a_playback_get_next_capture(m_handle, &capture_handle);

        if (K4A_STREAM_RESULT_SUCCEEDED == result)
        {
            *cap = capture(capture_handle);
            return true;
        }
        else if (K4A_STREAM_RESULT_EOF == result)
        {
            return false;
        }

        throw error("Failed to get next capture!");
    }

    /** Get the next capture in the recording
     * Returns true if a capture was available, false if there are none left
     * Throws error on failure.
     *
     * \sa k4a_playback_get_previous_capture
     */
    bool get_previous_capture(capture *cap)
    {
        k4a_capture_t capture_handle;
        k4a_stream_result_t result = k4a_playback_get_previous_capture(m_handle, &capture_handle);

        if (K4A_STREAM_RESULT_SUCCEEDED == result)
        {
            *cap = capture(capture_handle);
            return true;
        }
        else if (K4A_STREAM_RESULT_EOF == result)
        {
            return false;
        }

        throw error("Failed to get next capture!");
    }

    /** Reads the value of a tag from the recording
     * Returns false if the tag does not exist.
     *
     * \sa k4a_playback_get_tag
     */
    bool get_tag(const char *name, std::string *out) const
    {
        std::string tag;
        size_t buffer = 0;
        k4a_buffer_result_t result = k4a_playback_get_tag(m_handle, name, &tag[0], &buffer);

        if (result == K4A_BUFFER_RESULT_TOO_SMALL && buffer > 0)
        {
            tag.resize(buffer);
            result = k4a_playback_get_tag(m_handle, name, &tag[0], &buffer);
            if (result == K4A_BUFFER_RESULT_SUCCEEDED && tag[buffer - 1] == 0)
            {
                // std::string expects there to not be as null terminator at the end of its data but
                // k4a_playback_get_tag adds a null terminator, so we drop the last character of the string after we
                // get the result back.
                tag.resize(buffer - 1);
            }
        }

        if (result != K4A_BUFFER_RESULT_SUCCEEDED)
        {
            return false;
        }

        *out = std::move(tag);

        return true;
    }

    /** Get the next IMU sample in the recording
     * Returns true if a sample was available, false if there are none left
     * Throws error on failure.
     *
     * \sa k4a_playback_get_next_imu_sample
     */
    bool get_next_imu_sample(k4a_imu_sample_t *sample)
    {
        k4a_stream_result_t result = k4a_playback_get_next_imu_sample(m_handle, sample);

        if (K4A_STREAM_RESULT_SUCCEEDED == result)
        {
            return true;
        }
        else if (K4A_STREAM_RESULT_EOF == result)
        {
            return false;
        }

        throw error("Failed to get next IMU sample!");
    }

    /** Get the previous IMU sample in the recording
     * Returns true if a sample was available, false if there are none left
     * Throws error on failure.
     *
     * \sa k4a_playback_get_previous_imu_sample
     */
    bool get_previous_imu_sample(k4a_imu_sample_t *sample)
    {
        k4a_stream_result_t result = k4a_playback_get_previous_imu_sample(m_handle, sample);

        if (K4A_STREAM_RESULT_SUCCEEDED == result)
        {
            return true;
        }
        else if (K4A_STREAM_RESULT_EOF == result)
        {
            return false;
        }

        throw error("Failed to get previous IMU sample!");
    }

    /** Seeks to a specific time point in the recording
     * Throws error on failure.
     *
     * \sa k4a_playback_seek_timestamp
     */
    void seek_timestamp(std::chrono::microseconds offset, k4a_playback_seek_origin_t origin)
    {
        k4a_result_t result = k4a_playback_seek_timestamp(m_handle, offset.count(), origin);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to seek recording!");
        }
    }

    /** Get the last valid timestamp in the recording
     *
     * \sa k4a_playback_get_recording_length_usec
     */
    std::chrono::microseconds get_recording_length() const noexcept
    {
        return std::chrono::microseconds(k4a_playback_get_recording_length_usec(m_handle));
    }

    /** Set the image format that color captures will be converted to. By default the conversion format will be the same
     * as the image format stored in the recording file, and no conversion will occur.
     *
     * Throws error on failure.
     *
     * \sa k4a_playback_set_color_conversion
     */
    void set_color_conversion(k4a_image_format_t format)
    {
        k4a_result_t result = k4a_playback_set_color_conversion(m_handle, format);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to set color conversion!");
        }
    }

    /** Opens a K4A recording for playback.
     * Throws error on failure.
     *
     * \sa k4a_playback_open
     */
    static playback open(const char *path)
    {
        k4a_playback_t handle = nullptr;
        k4a_result_t result = k4a_playback_open(path, &handle);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to open recording!");
        }

        return playback(handle);
    }

private:
    k4a_playback_t m_handle;
};

} // namespace k4a

#endif
