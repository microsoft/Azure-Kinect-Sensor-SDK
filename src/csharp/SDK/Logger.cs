//------------------------------------------------------------------------------
// <copyright file="Logger.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using Microsoft.Azure.Kinect.Sensor.Native;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// The Azure Kinect logging system. Enables access to the debug messages from the Azure Kinect device.
    /// </summary>
    public static class Logger
    {
        private static readonly NativeMethods.k4a_logging_message_cb_t DebugMessageHandler = OnDebugMessage;
        private static EventHandler<DebugMessageEventArgs> logMessageHandlers;
        //private static TraceSource source = new TraceSource("AzureKinect");

        /// <summary>
        /// Occurs when the Azure Kinect Sensor SDK delivers a debug message.
        /// </summary>
        public static event EventHandler<DebugMessageEventArgs> LogMessage
        {
            add
            {
                if (!Initialized)
                {
                    Logger.Initialize();
                }

                logMessageHandlers += value;
            }

            remove
            {
                logMessageHandlers -= value;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the <see cref="Logger"/> class has been initialized and connected to the Azure Kinect Sensor SDK.
        /// </summary>
        public static bool Initialized { get; private set; }

        /// <summary>
        /// Initializes the <see cref="Logger"/> class to begin receiving messages from the Azure Kinect Sensor SDK.
        /// </summary>
        public static void Initialize()
        {
            lock (DebugMessageHandler)
            {
                if (Logger.Initialized)
                {
                    return;
                }

                AppDomain.CurrentDomain.ProcessExit += CurrentDomain_Exit;
                AppDomain.CurrentDomain.DomainUnload += CurrentDomain_Exit;
                NativeMethods.k4a_result_t result = NativeMethods.k4a_set_debug_message_handler(DebugMessageHandler, IntPtr.Zero, LogLevel.Trace);
                if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
                {
                    throw new AzureKinectException("Failed to set the Debug Message Handler");
                }

                //DisplayProperties(source);
                Logger.Initialized = true;
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
                    Console.WriteLine("AssemblyQualifiedName = " + traceListener.GetType().AssemblyQualifiedName);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Ooops, {0}", ex);
            }
        }

        private static void OnDebugMessage(IntPtr context, LogLevel level, string file, int line, string message)
        {
            DebugMessageEventArgs data = new DebugMessageEventArgs() { LogLevel = level, FileName = file, Line = line, Message = message };

            EventHandler<DebugMessageEventArgs> eventhandler = logMessageHandlers;
            if (eventhandler != null)
            {
                eventhandler(null, data);
            }

            // TODO: Add a default logging?
            //Console.WriteLine("{0} {1}@{2}: {3}", level, file, line, message);

            //source.TraceEvent(System.Diagnostics.TraceEventType.Error, )
            //    source.TraceData(TraceEventType.Transfer, )

            //TraceEventType traceEventType;
            //switch (level)
            //{
            //    case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_CRITICAL:
            //        traceEventType = TraceEventType.Critical;
            //        break;

            //    case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_ERROR:
            //        traceEventType = TraceEventType.Error;
            //        break;

            //    case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_WARNING:
            //        traceEventType = TraceEventType.Warning;
            //        break;

            //    case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_INFO:
            //        traceEventType = TraceEventType.Information;
            //        break;

            //    case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_TRACE:
            //        traceEventType = TraceEventType.Verbose;
            //        break;

            //    case NativeMethods.k4a_log_level_t.K4A_LOG_LEVEL_OFF:
            //    default:
            //        throw new AzureKinectException("Unexpected log level.");
            //}

            //// TODO: How do we want to format the trace event?
            //source.TraceEvent(traceEventType, 1, message);
        }

        private static void CurrentDomain_Exit(object sender, EventArgs e)
        {
            NativeMethods.k4a_result_t result = NativeMethods.k4a_set_debug_message_handler(null, IntPtr.Zero, LogLevel.Off);
            if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
            {
                Trace.WriteLine("Failed to close the debug message handler");
            }
        }
    }
}
