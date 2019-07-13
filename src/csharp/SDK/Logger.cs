using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Contracts;
using System.Diagnostics.Tracing;
using System.Runtime.CompilerServices;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Azure.Kinect.Sensor.Native;

namespace Microsoft.Azure.Kinect.Sensor
{
    public class LogMessageData
    {
        public string level { get; set; }
        public string file { get; set; }
        public int line { get; set; }
        public string message { get; set; }
    }

    public static class Logger
    {
        private static EventHandler<LogMessageData> logMessageHandlers;
        private static NativeMethods.k4a_logging_message_cb_t messageHandler = logHandler;
        private static TraceSource source = new TraceSource("AzureKinect");

        /// <summary>
        /// Initializes static members of the <see cref="Logger"/> class.
        /// </summary>
        //[SwitchAttribute("SourceSwitch", typeof(SourceSwitch))]
        static Logger()
        {
            AppDomain.CurrentDomain.ProcessExit += CurrentDomain_Exit;
            AppDomain.CurrentDomain.DomainUnload += CurrentDomain_Exit;
            NativeMethods.k4a_result_t result = NativeMethods.k4a_set_debug_message_handler(messageHandler, IntPtr.Zero, NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_WARNING);
            if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
            {
                throw new Exception();
            }

            DisplayProperties(source);
        }

        /// <summary>
        /// Event that occurs whenever a log message is received from the Azure Kinect Sensor SDK.
        /// </summary>
        public static event EventHandler<LogMessageData> LogMessage
        {
            add
            {
                logMessageHandlers += value;
            }

            remove
            {
                logMessageHandlers -= value;
            }
        }

        private static void DisplayProperties(TraceSource ts)
        {
            try
            {
                Console.WriteLine("TraceSource name = " + ts.Name);
                Console.WriteLine("TraceSource switch level = " + ts.Switch.Level);
                Console.WriteLine("TraceSource switch = " + ts.Switch.DisplayName);
                //SwitchAttribute[] switches = SwitchAttribute.GetAll(typeof(Logger).Assembly);
                //for (int i = 0; i < switches.Length; i++)
                //{
                //    Console.WriteLine("Switch name = " + switches[i].SwitchName);
                //    Console.WriteLine("Switch type = " + switches[i].SwitchType);
                //}

                // Get the custom attributes for the TraceSource.
                Console.WriteLine("Number of custom trace source attributes = " + ts.Attributes.Count);
                foreach (DictionaryEntry de in ts.Attributes)
                {
                    Console.WriteLine("Custom trace source attribute = " + de.Key + "  " + de.Value);
                }

                // Get the custom attributes for the trace source switch.
                foreach (DictionaryEntry de in ts.Switch.Attributes)
                {
                    Console.WriteLine("Custom switch attribute = " + de.Key + "  " + de.Value);
                }

                Console.WriteLine("Number of listeners = " + ts.Listeners.Count);
                foreach (TraceListener traceListener in ts.Listeners)
                {
                    Console.Write("TraceListener: " + traceListener.Name + "\t");
                    // The following output can be used to update the configuration file.
                    Console.WriteLine("AssemblyQualifiedName = " + (traceListener.GetType().AssemblyQualifiedName));
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Ooops, {0}", ex);
            }
        }

        private static void logHandler(IntPtr context, NativeMethods.k4a_log_level_t level, string file, int line, string message)
        {
            LogMessageData data = new LogMessageData() { level = level.ToString(), file = file, line = line, message = message };

            EventHandler<LogMessageData> eventhandler = logMessageHandlers;
            if (eventhandler != null)
            {
                eventhandler(null, data);
            }

            // TODO: Add a default logging?
            //Console.WriteLine("{0} {1}@{2}: {3}", level, file, line, message);

            //source.TraceEvent(System.Diagnostics.TraceEventType.Error, )
            //    source.TraceData(TraceEventType.Transfer, )

            TraceEventType traceEventType;
            switch (level)
            {
                case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_CRITICAL:
                    traceEventType = TraceEventType.Critical;
                    break;

                case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_ERROR:
                    traceEventType = TraceEventType.Error;
                    break;

                case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_WARNING:
                    traceEventType = TraceEventType.Warning;
                    break;

                case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_INFO:
                    traceEventType = TraceEventType.Information;
                    break;

                case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_TRACE:
                    traceEventType = TraceEventType.Verbose;
                    break;

                case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_OFF:
                default:
                    throw new AzureKinectException("Unexpected log level.");
            }

            source.TraceEvent(traceEventType, 1, message);
        }

        private static void CurrentDomain_Exit(object sender, EventArgs e)
        {
            NativeMethods.k4a_result_t result = NativeMethods.k4a_set_debug_message_handler(null, IntPtr.Zero, NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_OFF);
            if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
            {
                Debug.WriteLine("Failed to close the debug message handler");
            }
        }
    }
}
