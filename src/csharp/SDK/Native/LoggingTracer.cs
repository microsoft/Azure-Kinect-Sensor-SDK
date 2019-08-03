//------------------------------------------------------------------------------
// <copyright file="LoggingTracer.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents a tracer for capturing thread specific logging messages for tracing native calls
    /// into the Azure Kinect Sensor SDK.
    /// </summary>
    internal class LoggingTracer : IDisposable
    {
        /// <summary>
        /// Object for synchronizing access to the global tracer reference count and the event handler.
        /// </summary>
        private static readonly object SyncRoot = new object();

        /// <summary>
        /// The reference count for the total number of threads that are currently tracing logging messages.
        /// </summary>
        private static int tracerRefCount = 0;

        /// <summary>
        /// The number of references to the tracer for the current thread.
        /// </summary>
        [ThreadStatic]
        private static int threadRefCount = 0;

        /// <summary>
        /// The messages that were generated on the current thread while a tracer is active.
        /// </summary>
        [ThreadStatic]
        private static List<string> messages = null;

        private bool disposed;

        /// <summary>
        /// Initializes a new instance of the <see cref="LoggingTracer"/> class.
        /// </summary>
        public LoggingTracer()
        {
            if (threadRefCount == 0)
            {
                ++threadRefCount;
                messages = new List<string>();

                lock (SyncRoot)
                {
                    if (++tracerRefCount == 1)
                    {
                        try
                        {
                            // Only allow one event callback to be subscribe to prevent duplicate messages.
                            Logger.LogMessage += Logger_LogMessage;
                        }
                        catch (Exception)
                        {
                            // Clear out the ref count if this fails.
                            --tracerRefCount;
                            --threadRefCount;
                            throw;
                        }
                    }
                }
            }
            else
            {
                throw new InvalidOperationException("There is already a logging tracer on this thread.");
            }
        }

        /// <summary>
        /// Gets all of the messages that have occurred on this thread since the tracing began.
        /// </summary>
        public IList<string> LogMessages
        {
            get
            {
                if (this.disposed)
                {
                    throw new ObjectDisposedException(nameof(LoggingTracer));
                }

                return messages?.ToArray();
            }
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
        /// Releases unmanaged and - optionally - managed resources.
        /// </summary>
        /// <param name="disposing"><c>True</c> to release both managed and unmanaged resources; <c>False</c> to release only unmanaged resources.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    if (--threadRefCount == 0)
                    {
                        lock (SyncRoot)
                        {
                            if (--tracerRefCount == 0)
                            {
                                Logger.LogMessage -= Logger_LogMessage;
                            }
                        }

                        // There are no longer any tracers on this thread. Clear the message list to allow the memory to be freed.
                        messages = null;
                    }
                }

                this.disposed = true;
            }
        }

        private static void Logger_LogMessage(object sender, DebugMessageEventArgs e)
        {
            if (threadRefCount == 0)
            {
                // There are no active tracers on the current thread, ignore the message.
                return;
            }

            messages.Add($"{DateTime.Now} [{e.LogLevel}] {e.FileName}@{e.Line}: {e.Message}");
        }
    }
}
