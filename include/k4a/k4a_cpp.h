#pragma once

#include "k4a.h"

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

namespace k4a
{
    inline void check_result(k4a_result_t result)
    {
        if (result != K4A_RESULT_SUCCEEDED)
        {
            // TODO: Throw exception
        }
    }

    inline void check_result(k4a_wait_result_t result)
    {
        if (result != K4A_WAIT_RESULT_SUCCEEDED)
        {
            // TODO: Throw exception
        }
    }

    class image
    {
    public:
        image(k4a_image_t handle = nullptr)
            : m_handle(handle)
        {
        }

        image(const image& other)
            : m_handle(other.m_handle)
        {
            if (m_handle != nullptr)
                k4a_image_reference(m_handle);
        }

        image(image&& other)
            : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
        }

        ~image()
        {
            release();
        }

        image& operator=(const image& other)
        {
            if (this != &other)
            {
                release();
                m_handle = other.m_handle;
                if (m_handle != nullptr)
                    k4a_image_reference(m_handle);
            }
            return *this;
        }

        image& operator=(image&& other)
        {
            release();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
            return *this;
        }

        bool operator==(const image& other) const
        {
            return m_handle == other.m_handle;
        }

        bool operator==(std::nullptr_t) const
        {
            return m_handle == nullptr;
        }

        bool operator!=(const image& other) const
        {
            return m_handle == other.m_handle;
        }

        bool operator!=(std::nullptr_t) const
        {
            return m_handle == nullptr;
        }

        operator bool() const
        {
            return m_handle != nullptr;
        }

        void release()
        {
            if (m_handle != nullptr)
            {
                k4a_image_release(m_handle);
                m_handle = nullptr;
            }
        }

        static k4a_result_t create(k4a_image_format_t format, int width_pixels, int height_pixels, int stride_bytes, image& img)
        {
            k4a_image_t handle = nullptr;
            k4a_result_t result = k4a_image_create(format, width_pixels, height_pixels, stride_bytes, &handle);

            img = image(handle);
            return result;
        }

        static image create(k4a_image_format_t format, int width_pixels, int height_pixels, int stride_bytes)
        {
            image img;
            k4a_result_t result = create(format, width_pixels, height_pixels, stride_bytes, img);

            check_result(result);
            return img;
        }

        static k4a_result_t crate_from_buffer(
            k4a_image_format_t format,
            int width_pixels,
            int height_pixels,
            int stride_bytes,
            uint8_t *buffer,
            size_t buffer_size,
            k4a_memory_destroy_cb_t *buffer_release_cb,
            void *buffer_release_cb_context,
            image& img)
        {
            k4a_image_t handle = nullptr;
            k4a_result_t result = k4a_image_create_from_buffer(
                format,
                width_pixels,
                height_pixels,
                stride_bytes,
                buffer,
                buffer_size,
                buffer_release_cb,
                buffer_release_cb_context,
                &handle);

            img = image(handle);
            return result;
        }

        static image crate_from_buffer(
            k4a_image_format_t format,
            int width_pixels,
            int height_pixels,
            int stride_bytes,
            uint8_t *buffer,
            size_t buffer_size,
            k4a_memory_destroy_cb_t *buffer_release_cb,
            void *buffer_release_cb_context)
        {
            image img;
            k4a_result_t result = crate_from_buffer(format,
                width_pixels,
                height_pixels,
                stride_bytes,
                buffer,
                buffer_size,
                buffer_release_cb,
                buffer_release_cb_context,
                img);

            if (result != K4A_RESULT_SUCCEEDED)
            {
                // TODO: Throw exception
            }
            return img;
        }

        uint8_t* get_buffer() const
        {
            return k4a_image_get_buffer(m_handle);
        }

        size_t get_size() const
        {
            return k4a_image_get_size(m_handle);
        }

        k4a_image_format_t get_format() const
        {
            return k4a_image_get_format(m_handle);
        }

        int get_width_pixels() const
        {
            return k4a_image_get_width_pixels(m_handle);
        }

        int get_height_pixels() const
        {
            return k4a_image_get_height_pixels(m_handle);
        }

        int get_stride_bytes() const
        {
            return k4a_image_get_stride_bytes(m_handle);
        }

        std::chrono::microseconds get_timestamp() const
        {
            return std::chrono::microseconds(k4a_image_get_timestamp_usec(m_handle));
        }

        std::chrono::microseconds get_exposure() const
        {
            return std::chrono::microseconds(k4a_image_get_exposure_usec(m_handle));
        }

        uint32_t get_white_balance() const
        {
            return k4a_image_get_white_balance(m_handle);
        }

        uint32_t get_iso_speed() const
        {
            return k4a_image_get_iso_speed(m_handle);
        }

        void set_timestamp(std::chrono::microseconds timestamp)
        {
            k4a_image_set_timestamp_usec(m_handle, timestamp.count());
        }

        void set_exposure_time(std::chrono::microseconds timestamp)
        {
            k4a_image_set_exposure_time_usec(m_handle, timestamp.count());
        }

        // TODO: Other set methods

    private:
        k4a_image_t m_handle;
    };

    class capture
    {
    public:
        capture(k4a_capture_t handle = nullptr)
            : m_handle(handle)
        {
        }

        capture(capture&& other)
            : m_handle(other.m_handle)
        {
            other.m_handle = nullptr;
        }

        capture(const capture& other)
            : m_handle(other.m_handle)
        {
            if (m_handle != nullptr)
                k4a_capture_reference(m_handle);
        }

        ~capture()
        {
            release();
        }

        capture& operator=(const capture& other)
        {
            if (this != &other)
            {
                release();
                m_handle = other.m_handle;
                if (m_handle != nullptr)
                    k4a_capture_reference(m_handle);
            }
            return *this;
        }

        capture& operator=(capture&& other)
        {
            release();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
            return *this;
        }

        bool operator==(const capture& other) const
        {
            return m_handle == other.m_handle;
        }

        bool operator==(std::nullptr_t) const
        {
            return m_handle == nullptr;
        }

        bool operator!=(const capture& other) const
        {
            return m_handle == other.m_handle;
        }

        bool operator!=(std::nullptr_t) const
        {
            return m_handle == nullptr;
        }

        operator bool() const
        {
            return m_handle != nullptr;
        }

        void release()
        {
            if (m_handle != nullptr)
            {
                k4a_capture_release(m_handle);
                m_handle = nullptr;
            }
        }

        image get_color_image() const
        {
            return image(k4a_capture_get_color_image(m_handle));
        }

        image get_depth_image() const
        {
            return image(k4a_capture_get_depth_image(m_handle));
        }

        image get_ir_image() const
        {
            return image(k4a_capture_get_ir_image(m_handle));
        }

        // TODO:
        //void set_color_image(image img) // depth, ir
        //{
        //	k4a_capture_set_color_image(m_handle, img.handle());
        //}

        float get_temperature_c() const
        {
            return k4a_capture_get_temperature_c(m_handle);
        }

        void set_temperature_c(float temperature_c)
        {
            k4a_capture_set_temperature_c(m_handle, temperature_c);
        }

        static k4a_result_t create(capture& cap)
        {
            k4a_capture_t handle = nullptr;
            k4a_result_t result = k4a_capture_create(&handle);

            cap = capture(handle);
            return result;
        }

        static capture create()
        {
            capture cap;
            k4a_result_t result = create(cap);

            if (result != K4A_RESULT_SUCCEEDED)
            {
                // TODO: Throw exception
            }
            return cap;
        }

    private:
        k4a_capture_t m_handle;
    };

    class device
    {
    public:
        device(k4a_device_t handle = nullptr)
            : m_handle(handle)
        {
        }

        device(device&& dev)
            : m_handle(dev.m_handle)
        {
            dev.m_handle = nullptr;
        }

        device(const device&) = delete;

        ~device()
        {
            close();
        }

        device& operator=(const device&) = delete;

        device& operator=(device&& dev)
        {
            close();
            m_handle = dev.m_handle;
            dev.m_handle = nullptr;
            return *this;
        }

        operator bool() const
        {
            return m_handle != nullptr;
        }

        void close() noexcept
        {
            if (m_handle != nullptr)
            {
                k4a_device_close(m_handle);
                m_handle = nullptr;
            }
        }

        k4a_wait_result_t get_capture(capture& cap, std::chrono::milliseconds timeout)
        {
            k4a_capture_t capture_handle = nullptr;
            int64_t timeout_ms = timeout.count();
            if (timeout_ms < std::numeric_limits<int32_t>::min())
                timeout_ms = std::numeric_limits<int32_t>::min();
            else if (timeout_ms > std::numeric_limits<int32_t>::max())
                timeout_ms = std::numeric_limits<int32_t>::max();
            k4a_wait_result_t result = k4a_device_get_capture(m_handle, &capture_handle, (int32_t)timeout_ms);

            cap = capture(capture_handle);
            return result;
        }

        capture get_capture(std::chrono::milliseconds timeout)
        {
            capture cap;
            k4a_wait_result_t result = get_capture(cap, timeout);

            check_result(result);
            return cap;
        }

        k4a_wait_result_t get_imu_sample(k4a_imu_sample_t *imu_sample, std::chrono::milliseconds timeout)
        {
            int64_t timeout_ms = timeout.count();
            if (timeout_ms < std::numeric_limits<int32_t>::min())
                timeout_ms = std::numeric_limits<int32_t>::min();
            else if (timeout_ms > std::numeric_limits<int32_t>::max())
                timeout_ms = std::numeric_limits<int32_t>::max();
            return k4a_device_get_imu_sample(m_handle, imu_sample, (int32_t)timeout_ms);
        }

        k4a_imu_sample_t get_imu_sample(std::chrono::milliseconds timeout)
        {
            k4a_imu_sample_t imu_sample;
            k4a_wait_result_t result = get_imu_sample(&imu_sample, timeout);

            check_result(result);
            return imu_sample;
        }

        k4a_result_t start_cameras(const k4a_device_configuration_t& configuration)
        {
            return k4a_device_start_cameras(m_handle, const_cast<k4a_device_configuration_t*>(&configuration));
        }

        void stop_cameras()
        {
            k4a_device_stop_cameras(m_handle);
        }

        k4a_result_t start_imu()
        {
            return k4a_device_start_imu(m_handle);
        }

        void stop_imu()
        {
            k4a_device_stop_imu(m_handle);
        }

        k4a_buffer_result_t get_serialnum(char* serial_number, size_t* serial_number_size) const
        {
            return k4a_device_get_serialnum(m_handle, serial_number, serial_number_size);
        }

        std::string get_serialnum() const
        {
            std::string serialnum;
            size_t buffer = 0;
            k4a_buffer_result_t result = get_serialnum(&serialnum[0], &buffer);

            // On some prototype devices, we get back an empty serial number (just the '\0').
            // If that happens, we want to report 'unknown device' rather than not printing an identifier.
            //
            if (result == K4A_BUFFER_RESULT_TOO_SMALL && buffer > 1)
            {
                serialnum.resize(buffer);
                result = get_serialnum(&serialnum[0], &buffer);
                if (result == K4A_BUFFER_RESULT_SUCCEEDED)
                {
                    // std::string expects there to not be as null terminator at the end of its data but
                    // k4a_device_get_serialnum adds a null terminator, so we drop the last character of the string after we get
                    // the result back.
                    //
                    serialnum.resize(buffer - 1);
                }
            }

            if (result != K4A_BUFFER_RESULT_SUCCEEDED)
            {
                serialnum = "[Unknown K4A device]";
            }
            return serialnum;
        }

        k4a_result_t get_color_control(k4a_color_control_command_t command, k4a_color_control_mode_t *mode, int32_t *value) const
        {
            return k4a_device_get_color_control(m_handle, command, mode, value);
        }

        k4a_result_t set_color_control(k4a_color_control_command_t command, k4a_color_control_mode_t mode, int32_t value)
        {
            return k4a_device_set_color_control(m_handle, command, mode, value);
        }

        k4a_result_t get_sync_jack(bool *sync_in_jack_connected, bool *sync_out_jack_connected) const
        {
            return k4a_device_get_sync_jack(m_handle, sync_in_jack_connected, sync_out_jack_connected);
        }

        k4a_result_t get_version(k4a_hardware_version_t *version) const
        {
            return k4a_device_get_version(m_handle, version);
        }

        k4a_hardware_version_t get_version() const
        {
            k4a_hardware_version_t version;
            k4a_result_t result = get_version(&version);

            check_result(result);
            return version;
        }

        static k4a_result_t open(uint8_t index, device& dev) noexcept
        {
            k4a_device_t handle = nullptr;
            k4a_result_t result = k4a_device_open(index, &handle);

            dev = device(handle);
            return result;
        }

        static device open(uint8_t index)
        {
            device dev;
            k4a_result_t result = open(index, dev);

            check_result(result);
            return dev;
        }

        static uint32_t get_installed_count() noexcept
        {
            return k4a_device_get_installed_count();
        }

    private:
        k4a_device_t m_handle;
    };
}
