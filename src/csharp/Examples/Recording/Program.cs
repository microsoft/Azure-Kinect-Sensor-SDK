using System;
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
                DepthMode = DepthMode.NFOV_2x2Binned
            };

            using (Device device = Device.Open())
            using (Record recording = Record.Create(@"output.mkv", device, configuration))
            {
                device.StartCameras(configuration);

                recording.WriteHeader();

                for (frame = 0; frame < 100; frame++)
                {
                    using (Capture capture = device.GetCapture())
                    {
                        recording.WriteCapture(capture);
                    }
                }
            }

            Console.WriteLine($"Wrote {frame} frames to output.mkv");

            using (Playback playback = Playback.Open(@"output.mkv"))
            {
                Console.WriteLine($"Tracks = {playback.TrackCount}");
                
                for (int i = 0; i < playback.TrackCount; i++)
                {
                    string name = playback.GetTrackName(i);
                    string codecId = playback.GetTrackCodecId(name);

                    Console.WriteLine($"  Track {i}: {name} ({codecId}) (builtin={playback.GetTrackIsBuiltin(name)}");
                }
                Capture capture;
                while ( null != (capture = playback.GetNextCapture()))
                {
                    Console.WriteLine($"Depth timestamp: {capture.Depth.DeviceTimestamp}");
                        
                }
            }
        }
    }
}
