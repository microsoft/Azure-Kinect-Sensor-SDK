//------------------------------------------------------------------------------
// <copyright file="Logger.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections;
using System.Diagnostics;
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

                Logger.Initialized = true;
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
