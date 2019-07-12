using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Azure.Kinect.Sensor.Native;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class Logger : IDisposable
    {
        private static Logger defaultLogger;
        private bool disposed;

        private Logger()
        {
        }

        public static Logger GetDefaultLogger()
        {
            if (defaultLogger == null)
            {
                defaultLogger = new Logger();
            }

            return defaultLogger;
        }

        private static void logHandler(IntPtr context, NativeMethods.k4a_log_level_t level, string file, int line, string message)
        {
            Console.WriteLine("{0}@{1}: {2}", file, line, message);
        }

        private void Initialize()
        {
            NativeMethods.logger_register_message_callback(logHandler, IntPtr.Zero, NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_TRACE);
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Releases unmanaged and optionally managed resources.
        /// </summary>
        /// <param name="disposing"><c>true</c> to release both managed and unmanaged resources; <c>false</c> to release only unmanaged resources.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    // Release managed resources here.
                }

                // Release native unmanaged resources here.

                this.disposed = true;
            }
        }
    }
}
