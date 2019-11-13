/** \file record.hpp
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 * Kinect For Azure SDK - C++ wrapper.
 */
#ifndef K4A_RECORD_HPP
#define K4A_RECORD_HPP

#include <k4a/k4a.hpp>
#include <k4arecord/record.h>

namespace k4a
{

/** \class record record.hpp
 * Wrapper for \ref k4a_record_t
 *
 * Wraps a handle for a record object
 *
 * \sa k4a_record_t
 */
class record
{
public:
    /** Creates a k4a::record from a k4a_record_t
     * Takes ownership of the handle, i.e. you should not call
     * k4a_record_close on the handle after giving it to the
     * k4a::record; the k4a::record will take care of that.
     */
    record(k4a_record_t handle = nullptr) noexcept : m_handle(handle) {}

    /** Moves another k4a::record into a new k4a::record
     */
    record(record &&other) noexcept : m_handle(other.m_handle)
    {
        other.m_handle = nullptr;
    }

    record(const record &) = delete;

    ~record()
    {
        // NOTE: flush is called internally when closing record
        close();
    }

    record &operator=(const record &) = delete;

    /** Moves another k4a::record into this k4a::record; other is set to invalid
     */
    record &operator=(record &&other) noexcept
    {
        if (this != &other)
        {
            close();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }

        return *this;
    }

    /** Returns true if the k4a::record is valid, false otherwise
     */
    operator bool() const noexcept
    {
        return m_handle != nullptr;
    }

<<<<<<< HEAD
    /** Returns true if two recorder objects refer to the same k4a_record_t, false otherwise
     */
    bool operator==(const record &other) const noexcept
    {
        return m_handle == other.m_handle;
    }

    /** Returns false if the record object is valid, true otherwise
     */
    bool operator==(std::nullptr_t) const noexcept
    {
        return m_handle == nullptr;
    }

    /** Returns true if two record objects wrap different k4a_record_t instances, false otherwise
     */
    bool operator!=(const record &other) const noexcept
    {
        return m_handle != other.m_handle;
    }

    /** Returns true if the record object is valid, false otherwise
     */
    bool operator!=(std::nullptr_t) const noexcept
    {
        return m_handle != nullptr;
    }

=======
>>>>>>> upstream/develop
    /** Closes a K4A recording.
     *
     * \sa k4a_record_close
     */
    void close() noexcept
    {
<<<<<<< HEAD
        if (m_handle != nullptr)
=======
        if (m_handle)
>>>>>>> upstream/develop
        {
            k4a_record_close(m_handle);
            m_handle = nullptr;
        }
    }

<<<<<<< HEAD
    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
=======
    /** Flushes all pending recording data to disk
     *
     * \sa k4a_record_flush
     */
    void flush()
    {
        if (m_handle)
        {
            k4a_result_t result = k4a_record_flush(m_handle);

            if (K4A_RESULT_SUCCEEDED != result)
            {
                throw error("Failed to flush!");
            }
        }
    }

    /** Adds a tag to the recording
     * Throws error on failure
>>>>>>> upstream/develop
     *
     * \sa k4a_record_add_tag
     */
    void add_tag(const char *name, const char *value)
    {
        k4a_result_t result = k4a_record_add_tag(m_handle, name, value);

<<<<<<< HEAD
        if (K4A_FAILED(result))
=======
        if (K4A_RESULT_SUCCEEDED != result)
>>>>>>> upstream/develop
        {
            throw error("Failed to add tag!");
        }
    }

<<<<<<< HEAD
    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
=======
    /** Adds the track header for recording IMU
     * Throws error on failure
     *
     * \sa k4a_record_add_imu_track
>>>>>>> upstream/develop
     */
    void add_imu_track()
    {
        k4a_result_t result = k4a_record_add_imu_track(m_handle);

<<<<<<< HEAD
        if (K4A_FAILED(result))
        {
            throw error("Failed to add imu track!");
        }
    }

    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
=======
        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to add imu_track!");
        }
    }

    /** Adds an attachment to the recording
     * Throws error on failure
     *
     * \sa k4a_record_add_attachment
>>>>>>> upstream/develop
     */
    void add_attachment(const char *attachment_name, const uint8_t *buffer, size_t buffer_size)
    {
        k4a_result_t result = k4a_record_add_attachment(m_handle, attachment_name, buffer, buffer_size);

<<<<<<< HEAD
        if (K4A_FAILED(result))
=======
        if (K4A_RESULT_SUCCEEDED != result)
>>>>>>> upstream/develop
        {
            throw error("Failed to add attachment!");
        }
    }

<<<<<<< HEAD
    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
=======
    /** Adds custom video tracks to the recording
     * Throws error on failure
     *
     * \sa k4a_record_add_custom_video_track
>>>>>>> upstream/develop
     */
    void add_custom_video_track(const char *track_name,
                                const char *codec_id,
                                const uint8_t *codec_context,
                                size_t codec_context_size,
                                const k4a_record_video_settings_t *track_settings)
    {
<<<<<<< HEAD
        k4a_result_t result;
        result = k4a_record_add_custom_video_track(m_handle,
                                                   track_name,
                                                   codec_id,
                                                   codec_context,
                                                   codec_context_size,
                                                   track_settings);

        if (K4A_FAILED(result))
        {
            throw error("Failed to add video track!");
        }
    }

    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
=======
        k4a_result_t result = k4a_record_add_custom_video_track(m_handle,
                                                                track_name,
                                                                codec_id,
                                                                codec_context,
                                                                codec_context_size,
                                                                track_settings);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to add custom video track!");
        }
    }

    /** Adds custom subtitle tracks to the recording
     * Throws error on failure
     *
     * \sa k4a_record_add_custom_subtitle_track
>>>>>>> upstream/develop
     */
    void add_custom_subtitle_track(const char *track_name,
                                   const char *codec_id,
                                   const uint8_t *codec_context,
                                   size_t codec_context_size,
                                   const k4a_record_subtitle_settings_t *track_settings)
    {
<<<<<<< HEAD
        k4a_result_t result;
        result = k4a_record_add_custom_subtitle_track(m_handle,
                                                      track_name,
                                                      codec_id,
                                                      codec_context,
                                                      codec_context_size,
                                                      track_settings);

        if (K4A_FAILED(result))
=======
        k4a_result_t result = k4a_record_add_custom_subtitle_track(m_handle,
                                                                   track_name,
                                                                   codec_id,
                                                                   codec_context,
                                                                   codec_context_size,
                                                                   track_settings);

        if (K4A_RESULT_SUCCEEDED != result)
>>>>>>> upstream/develop
        {
            throw error("Failed to add custom subtitle track!");
        }
    }

<<<<<<< HEAD
    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
     */
    void write_header()
    {
        k4a_result_t result;
        result = k4a_record_write_header(m_handle);

        if (K4A_FAILED(result))
=======
    /** Writes the recording header and metadata to file
     * Throws error on failure
     *
     * \sa k4a_record_write_header
     */
    void write_header()
    {
        k4a_result_t result = k4a_record_write_header(m_handle);

        if (K4A_RESULT_SUCCEEDED != result)
>>>>>>> upstream/develop
        {
            throw error("Failed to write header!");
        }
    }

<<<<<<< HEAD
    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
     */
    void write_capture(capture capture)
    {
        k4a_result_t result;
        result = k4a_record_write_capture(m_handle, capture.handle());

        if (K4A_FAILED(result))
=======
    /** Writes a camera capture to file
     * Throws error on failure
     *
     * \sa k4a_record_write_capture
     */
    void write_capture(capture &capture)
    {
        k4a_result_t result = k4a_record_write_capture(m_handle, capture.handle());

        if (K4A_RESULT_SUCCEEDED != result)
>>>>>>> upstream/develop
        {
            throw error("Failed to write capture!");
        }
    }

<<<<<<< HEAD
    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
     */
    void write_imu_sample(k4a_imu_sample_t imu_sample)
    {
        k4a_result_t result;
        result = k4a_record_write_imu_sample(m_handle, imu_sample);

        if (K4A_FAILED(result))
        {
            throw error("Failed to write IMU sample!");
        }
    }

    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
     */
    void write_custom_track_data(const char *track_name,
                                 std::chrono::microseconds timestamp,
                                 uint8_t *custom_data,
                                 size_t custom_data_size)
    {
        k4a_result_t result;
        result =
            k4a_record_write_custom_track_data(m_handle, track_name, timestamp.count(), custom_data, custom_data_size);

        if (K4A_FAILED(result))
=======
    /** Writes an imu sample to file
     * Throws error on failure
     *
     * \sa k4a_record_write_imu_sample
     */
    void write_imu_sample(k4a_imu_sample_t &imu_sample)
    {
        k4a_result_t result = k4a_record_write_imu_sample(m_handle, imu_sample);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to write imu sample!");
        }
    }

    /** Writes data for a custom track to file
     * Throws error on failure
     *
     * \sa k4a_record_write_custom_track_data
     */
    void write_custom_track_data(const char *track_name,
                                 uint64_t device_timestamp_usec,
                                 uint8_t *custom_data,
                                 size_t custom_data_size)
    {
        k4a_result_t result = k4a_record_write_custom_track_data(m_handle,
                                                                 track_name,
                                                                 device_timestamp_usec,
                                                                 custom_data,
                                                                 custom_data_size);

        if (K4A_RESULT_SUCCEEDED != result)
>>>>>>> upstream/develop
        {
            throw error("Failed to write custom track data!");
        }
    }

<<<<<<< HEAD
    /** ???? Get the raw calibration blob for the K4A device that made the recording.
     * Throws error on failure.
     *
     * \sa k4a_record_add_tag
     */
    void flush() const
    {
        k4a_result_t result;
        result = k4a_record_flush(m_handle);

        if (K4A_FAILED(result))
        {
            throw error("Failed to flush!");
        }
    }

    /** Opens a K4A recording for record.
     * Throws error on failure.
     *
     * \sa k4a_record_open
     */
    static record create(const char *path, device device, const k4a_device_configuration_t *device_config)
    {
        k4a_record_t handle = nullptr;
        k4a_result_t result = k4a_record_create(path, device.handle(), *device_config, &handle);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to open recording!");
=======
    /** Opens a new recording file for writing
     * Throws error on failure
     *
     * \sa k4a_record_create
     */
    static record create(const char *path, device &device, k4a_device_configuration_t &device_configuration)
    {
        k4a_record_t handle = nullptr;
        k4a_result_t result = k4a_record_create(path, device.handle(), device_configuration, &handle);

        if (K4A_RESULT_SUCCEEDED != result)
        {
            throw error("Failed to create recorder!");
>>>>>>> upstream/develop
        }

        return record(handle);
    }

private:
    k4a_record_t m_handle;
};

} // namespace k4a

#endif
