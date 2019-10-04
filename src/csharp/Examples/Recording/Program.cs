using System;
using System.Linq.Expressions;
using Microsoft.Azure.Kinect.Sensor;
using Microsoft.Azure.Kinect.Sensor.Record;

namespace Recording
{
    class Program
    {
        static void Main(string[] args)
        {
            int frame = 0;

            Console.WriteLine("Recording from device.");

            DeviceConfiguration configuration = new DeviceConfiguration()
            {
                CameraFPS = FPS.FPS30,
                ColorFormat = ImageFormat.ColorMJPG,
                ColorResolution = ColorResolution.R720p,
                DepthMode = DepthMode.NFOV_2x2Binned,
                SynchronizedImagesOnly = true
            };

            using (Device device = Device.Open())
            using (Record recording = Record.Create(@"output.mkv", device, configuration))
            {
                
                device.StartCameras(configuration);
                device.StartImu();

                recording.AddImuTrack();
                recording.WriteHeader();

                for (frame = 0; frame < 100; frame++)
                {
                    using (Capture capture = device.GetCapture())
                    {
                        recording.WriteCapture(capture);
                        Console.WriteLine($"Wrote capture ({capture.Color.DeviceTimestamp})");
                        try
                        {
                            while (true)
                            {
                                // Throws TimeoutException when Imu sample is not available
                                ImuSample sample = device.GetImuSample(TimeSpan.Zero);

                                recording.WriteImuSample(sample);
                                Console.WriteLine($"Wrote imu ({sample.AccelerometerTimestamp})");
                            }
                        }catch (TimeoutException)
                        {

                        }
                    }
                }
            }

            Console.WriteLine($"Wrote {frame} frames to output.mkv");

            using (Playback playback = Playback.Open(@"output.mkv"))
            {
                Console.WriteLine($"Tracks = {playback.TrackCount}");
                Console.WriteLine($"RecordingLength = {playback.RecordingLength}");

                for (int i = 0; i < playback.TrackCount; i++)
                {
                    string name = playback.GetTrackName(i);
                    string codecId = playback.GetTrackCodecId(name);

                    Console.WriteLine($"  Track {i}: {name} ({codecId}) (builtin={playback.GetTrackIsBuiltin(name)})");
                }
                Capture capture;
                while ( null != (capture = playback.GetNextCapture()))
                {
                    Console.WriteLine($"Color timestamp: {capture.Color.DeviceTimestamp}  Depth timestamp: {capture.Depth.DeviceTimestamp}");                        
                }
            }
        }
    }
}
