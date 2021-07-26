﻿//------------------------------------------------------------------------------
// <copyright file="Logger.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Diagnostics;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// The Azure Kinect logging system. Enables access to the debug messages from the Azure Kinect device.
    /// </summary>
    public static class Logger
    {
        private static readonly object SyncRoot = new object();
        private static readonly NativeMethods.k4a_logging_message_cb_t DebugMessageHandler = OnDebugMessage;
        private static readonly LoggerProvider LoggerProviderValue = new LoggerProvider();
        private static bool isInitialized;

#pragma warning disable CA1003 // Use generic event handler instances
        /// <summary>
        /// Occurs when the Azure Kinect Sensor SDK delivers a debug message.
        /// </summary>
        public static event Action<LogMessage> LogMessage
#pragma warning restore CA1003 // Use generic event handler instances
        {
            add
            {
                lock (SyncRoot)
                {
                    if (!Logger.isInitialized)
                    {
                        Logger.Initialize();
                    }

                    LogMessageHandlers += value;
                }
            }

            remove
            {
                lock (SyncRoot)
                {
                    LogMessageHandlers -= value;
                }
            }
        }

        private static event Action<LogMessage> LogMessageHandlers;

        /// <summary>
        /// Gets the interface for reading log messages.
        /// </summary>
        public static ILoggingProvider LogProvider
        {
            get
            {
                return Logger.LoggerProviderValue;
            }
        }

        /// <summary>
        /// Initializes the <see cref="Logger"/> class to begin receiving messages from the Azure Kinect Sensor SDK.
        /// </summary>
        public static void Initialize()
        {
            lock (SyncRoot)
            {
                if (Logger.isInitialized)
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

                Logger.isInitialized = true;
            }
        }

        /// <summary>
        /// Resets the logger to an uninitialized state. This is used in the Unit Tests to ensure that the
        /// initialization is run during the unit tests.
        /// </summary>
        internal static void Reset()
        {
            lock (SyncRoot)
            {
                if (!Logger.isInitialized)
                {
                    return;
                }

                AppDomain.CurrentDomain.ProcessExit -= CurrentDomain_Exit;
                AppDomain.CurrentDomain.DomainUnload -= CurrentDomain_Exit;

                // TODO: This won't work as Raise Error has an invalid pointer.
                ////NativeMethods.k4a_result_t result = NativeMethods.k4a_set_debug_message_handler(null, IntPtr.Zero, LogLevel.Trace);
                ////if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
                ////{
                ////    throw new AzureKinectException("Failed to set the Debug Message Handler");
                ////}

                Logger.isInitialized = false;
            }
        }

        private static void OnDebugMessage(IntPtr context, LogLevel level, string file, int line, string message)
        {
            LogMessage data = new LogMessage(DateTime.Now, level, file, line, message);

            Action<LogMessage> eventhandler = LogMessageHandlers;
            if (eventhandler != null)
            {
                eventhandler(data);
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

        private class LoggerProvider : ILoggingProvider
        {
            public event Action<LogMessage> LogMessage
            {
                add
                {
                    Logger.LogMessage += value;
                }

                remove
                {
                    Logger.LogMessage -= value;
                }
            }

            public string ProviderName => "Azure Kinect SDK";
        }
    }
}
