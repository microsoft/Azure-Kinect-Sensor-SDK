using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Azure.Kinect.Sensor.Record.Exceptions;

namespace Microsoft.Azure.Kinect.Sensor.Record
{
    public class Playback : IDisposable
    {
        // The native handle for this recording.
        private readonly NativeMethods.k4a_playback_t handle;

        // To detect redundant calls to Dispose
        private bool disposedValue = false;

        // Cached values
        private Calibration? calibration = null;
        private RecordConfiguration recordConfiguration = null;

        private Playback(NativeMethods.k4a_playback_t handle)
        {
            this.handle = handle;
        }

        /// <summary>
        /// Opens an existing recording file for reading.
        /// </summary>
        /// <param name="path">Filesystem path of the existing recording.</param>
        /// <returns></returns>
        public static Playback Open(string path)
        {
            NativeMethods.k4a_playback_t handle = null;
            
            AzureKinectOpenPlaybackException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_open(path, out handle));
            
            return new Playback(handle);
        }

        /// <summary>
        /// Get the raw calibration blob for the Azure Kinect device used during recording.
        /// </summary>
        /// <returns>The raw calibration may not exist if the device was not specified during recording.</returns>
        public byte[] GetRawCalibration()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                // Determine the required calibration size
                UIntPtr size = new UIntPtr(0);
                if (NativeMethods.k4a_playback_get_raw_calibration(this.handle, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                {
                    throw new AzureKinectGetRawCalibrationException($"Unexpected result calling {nameof(NativeMethods.k4a_playback_get_raw_calibration)}");
                }

                // Allocate a string buffer
                byte[] raw = new byte[size.ToUInt32()];

                // Get the raw calibration
                AzureKinectGetRawCalibrationException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_get_raw_calibration(this.handle, raw, ref size));

                return raw;
            }
        }

        /// <summary>
        /// Get the camera calibration for Azure Kinect device used during recording. The output struct is used as input to all transformation functions.
        /// </summary>
        /// <remarks>
        /// The calibration may not exist if the device was not specified during recording.
        /// </remarks>
        public Calibration? Calibration
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Playback));
                    }

                    if (!this.calibration.HasValue)
                    {
                        Calibration localCalibration = new Calibration();
                        if (NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED == NativeMethods.k4a_playback_get_calibration(this.handle, out localCalibration))
                        {
                            this.calibration = localCalibration;
                        }
                    }

                    return this.calibration;
                }
            }
        }
        
        public RecordConfiguration RecordConfiguration
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Playback));
                    }

                    if (this.recordConfiguration == null)
                    {
                        if (NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED == NativeMethods.k4a_playback_get_record_configuration(this.handle, out NativeMethods.k4a_record_configuration_t nativeConfig))
                        {
                            this.recordConfiguration = RecordConfiguration.FromNative(nativeConfig);
                        }
                    }

                    return this.recordConfiguration;
                }
            }
        }

        public bool CheckTrackExists(string trackName)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (trackName == null)
                {
                    throw new ArgumentNullException(nameof(trackName));
                }

                return NativeMethods.k4a_playback_check_track_exists(this.handle, trackName);
            }
        }
        public int TrackCount
        {
            get
            {
                lock (this)
                {
                    if (this.disposedValue)
                    {
                        throw new ObjectDisposedException(nameof(Playback));
                    }

                    return checked((int)NativeMethods.k4a_playback_get_track_count(this.handle));
                }
            }
        }

        public string GetTrackName(int index)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (index >= this.TrackCount || index < 0)
                {
                    throw new ArgumentOutOfRangeException(nameof(index));
                }

                // Determine the required string size
                UIntPtr size = new UIntPtr(0);
                if (NativeMethods.k4a_playback_get_track_name(this.handle, (UIntPtr)index, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                {
                    throw new AzureKinectException($"Unexpected internal state calling {nameof(NativeMethods.k4a_playback_get_track_name)}");
                }

                // Allocate a string buffer
                StringBuilder trackname = new StringBuilder((int)size.ToUInt32());

                // Get the track name
                AzureKinectGetTrackNameException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_get_track_name(this.handle, (UIntPtr)index, trackname, ref size));

                return trackname.ToString();
            }
        }

        public bool GetTrackIsBuiltin(string trackName)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (trackName == null)
                {
                    throw new ArgumentNullException(nameof(trackName));
                }

                return NativeMethods.k4a_playback_track_is_builtin(this.handle, trackName);
            }
        }

        RecordVideoSettings GetTrackVideoSettings(string trackName)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (trackName == null)
                {
                    throw new ArgumentNullException(nameof(trackName));
                }

                RecordVideoSettings recordVideoSettings = new RecordVideoSettings();

                AzureKinectTrackGetVideoSettingsException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_track_get_video_settings(this.handle, trackName, out recordVideoSettings));

                return recordVideoSettings;
            }
        }

        public string GetTrackCodecId(string trackName)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (trackName == null)
                {
                    throw new ArgumentNullException(nameof(trackName));
                }

                // Determine the required string size
                UIntPtr size = new UIntPtr(0);
                if (NativeMethods.k4a_playback_track_get_codec_id(this.handle, trackName, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                {
                    throw new AzureKinectException($"Unexpected internal state calling {nameof(NativeMethods.k4a_playback_get_track_name)}");
                }

                // Allocate a string buffer
                StringBuilder codec = new StringBuilder((int)size.ToUInt32());

                // Get the codec id
                AzureKinectGetTrackNameException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_track_get_codec_id(this.handle, trackName, codec, ref size));

                return codec.ToString();
            }
        }

        public byte[] GetTrackCodecContext(string trackName)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (trackName == null)
                {
                    throw new ArgumentNullException(nameof(trackName));
                }

                // Determine the required buffer size
                UIntPtr size = new UIntPtr(0);
                if (NativeMethods.k4a_playback_track_get_codec_context(this.handle, trackName, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                {
                    throw new AzureKinectException($"Unexpected internal state calling {nameof(NativeMethods.k4a_playback_get_track_name)}");
                }

                // Allocate a buffer
                byte[] context = new byte[checked((int)size)];

                // Get the codec id
                AzureKinectGetTrackNameException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_track_get_codec_context(this.handle, trackName, context, ref size));

                return context;
            }
        }

        public string GetTag(string name)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (name == null)
                {
                    throw new ArgumentNullException(nameof(name));
                }

                // Determine the required string size
                UIntPtr size = new UIntPtr(0);
                if (NativeMethods.k4a_playback_get_tag(this.handle, name, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                {
                    throw new AzureKinectException($"Unexpected internal state calling {nameof(NativeMethods.k4a_playback_get_track_name)}");
                }

                // Allocate a string buffer
                StringBuilder value = new StringBuilder((int)size.ToUInt32());

                // Get the codec id
                AzureKinectGetTagException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_get_tag(this.handle, name, value, ref size));

                return value.ToString();
            }
        }

        public void SetColorConversion(ImageFormat targetFormat)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                AzureKinectSetColorConversionException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_set_color_conversion(this.handle, targetFormat));
            }
        }

        public byte[] GetAttachment(string fileName)
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                if (fileName == null)
                {
                    throw new ArgumentNullException(nameof(fileName));
                }

                // Determine the required buffer size
                UIntPtr size = new UIntPtr(0);
                if (NativeMethods.k4a_playback_get_attachment(this.handle, fileName, null, ref size) != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_TOO_SMALL)
                {
                    throw new AzureKinectException($"Unexpected internal state calling {nameof(NativeMethods.k4a_playback_get_track_name)}");
                }

                // Allocate a buffer
                byte[] buffer= new byte[checked((int)size)];

                // Get the codec id
                AzureKinectGetTrackNameException.ThrowIfNotSuccess(() => NativeMethods.k4a_playback_get_attachment(this.handle, fileName, buffer, ref size));

                return buffer;
            }
        }

        public Capture GetNextCapture()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                IntPtr captureHandle = IntPtr.Zero;

                switch (NativeMethods.k4a_playback_get_next_capture(this.handle, out captureHandle))
                {
                    case NativeMethods.k4a_stream_result_t.K4A_STREAM_RESULT_EOF:
                        return null;
                    case NativeMethods.k4a_stream_result_t.K4A_STREAM_RESULT_FAILED:
                        throw new AzureKinectGetCaptureException();
                    case NativeMethods.k4a_stream_result_t.K4A_STREAM_RESULT_SUCCEEDED:
                        return new Capture(captureHandle);
                }

                return null;
            }
        }

        public Capture GetPreviousCapture()
        {
            lock (this)
            {
                if (this.disposedValue)
                {
                    throw new ObjectDisposedException(nameof(Playback));
                }

                IntPtr captureHandle = IntPtr.Zero;

                switch (NativeMethods.k4a_playback_get_previous_capture(this.handle, out captureHandle))
                {
                    case NativeMethods.k4a_stream_result_t.K4A_STREAM_RESULT_EOF:
                        return null;
                    case NativeMethods.k4a_stream_result_t.K4A_STREAM_RESULT_FAILED:
                        throw new AzureKinectGetCaptureException();
                    case NativeMethods.k4a_stream_result_t.K4A_STREAM_RESULT_SUCCEEDED:
                        return new Capture(captureHandle);
                }

                return null;
            }
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            this.Dispose(true);

            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Handle the disposing of the object.
        /// </summary>
        /// <param name="disposing">true when called by Dispose(), false when called by the finalizer.</param>
        protected virtual void Dispose(bool disposing)
        {
            lock (this)
            {
                this.handle.Close();

                this.disposedValue = true;
            }
        }
    }
}
