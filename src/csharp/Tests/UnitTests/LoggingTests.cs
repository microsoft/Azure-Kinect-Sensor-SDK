//------------------------------------------------------------------------------
// <copyright file="LoggingTests.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Azure.Kinect.Sensor.Test.StubGenerator;
using NUnit.Framework;

namespace Microsoft.Azure.Kinect.Sensor.UnitTests
{
    /// <summary>
    /// These tests validate the calling and marshaling for the logging and logging tracing
    /// features of the C# wrapper for the Azure Kinect SDK.
    /// </summary>
    [TestFixture]
    public class LoggingTests : IDisposable
    {
        private readonly StubbedModule nativeK4a;

        private bool disposed;

        /// <summary>
        /// Initializes a new instance of the <see cref="LoggingTests"/> class.
        /// </summary>
        public LoggingTests()
        {
            this.nativeK4a = StubbedModule.Get("k4a");
            if (this.nativeK4a == null)
            {
                NativeInterface k4ainterface = NativeInterface.Create(
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.dll"),
                    EnvironmentInfo.CalculateFileLocation(@"k4a\k4a.h"));

                this.nativeK4a = StubbedModule.Create("k4a", k4ainterface);
            }
        }

        /// <summary>
        /// Initializes the <see cref="LoggingTests"/> class for running test cases.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            // Don't hook the native allocator
            Microsoft.Azure.Kinect.Sensor.Allocator.Singleton.UseManagedAllocator = false;

            // Clear the logger initialization state to make sure we re-register the logger
            // for each test.
            Logger.Reset();
        }

        /// <summary>
        /// Performs a basic set of tests to verify until the <see cref="Logger"/> is hooked that
        /// calls aren't made and that all of the data is marshaled through as expected.
        /// </summary>
        [Test]
        public void LoggerTest()
        {
            int calledCount = 0;
            this.SetMessageHandlerImplementation();

            void EventHandler(LogMessage l)
            {
                ++calledCount;
                Assert.AreEqual(LogLevel.Warning, l.LogLevel);
                Assert.AreEqual("File", l.FileName);
                Assert.AreEqual(1234, l.Line);
                Assert.AreEqual("Message", l.Message);
            }

            try
            {
                _ = Device.GetInstalledCount();
                Assert.AreEqual(0, calledCount);

                Logger.LogMessage += EventHandler;

                _ = Device.GetInstalledCount();
                Assert.AreEqual(1, calledCount);

                _ = Device.GetInstalledCount();
                Assert.AreEqual(2, calledCount);
            }
            finally
            {
                Logger.LogMessage -= EventHandler;
            }
        }

        /// <summary>
        /// Performs a basic set of tests to verify that the <see cref="LoggingTracer"/> is capable
        /// of capturing log messages correctly and clean up after they are done.
        /// </summary>
        [Test]
        public void LoggingTracerTest()
        {
            this.SetMessageHandlerImplementation();

            using (LoggingTracer tracer = new LoggingTracer())
            {
                Assert.AreEqual(0, Device.GetInstalledCount());
                Assert.AreEqual(1, tracer.LogMessages.Count);

                Assert.AreEqual(LogLevel.Warning, tracer.LogMessages[0].LogLevel);
                Assert.AreEqual("File", tracer.LogMessages[0].FileName);
                Assert.AreEqual(1234, tracer.LogMessages[0].Line);
                Assert.AreEqual("Message", tracer.LogMessages[0].Message);
            }

            // Make sure logging getting called without a logging tracer doesn't cause a failure.
            Assert.AreEqual(0, Device.GetInstalledCount());

            // Make sure that we can instantiate another logging tracer.
            using (LoggingTracer tracer = new LoggingTracer())
            {
                using (LoggingTracer tracer2 = new LoggingTracer())
                {
                    Assert.AreEqual(0, Device.GetInstalledCount());
                    Assert.AreEqual(1, tracer2.LogMessages.Count);
                }

                Assert.AreEqual(1, tracer.LogMessages.Count);
                Assert.AreEqual(LogLevel.Warning, tracer.LogMessages[0].LogLevel);
                Assert.AreEqual("File", tracer.LogMessages[0].FileName);
                Assert.AreEqual(1234, tracer.LogMessages[0].Line);
                Assert.AreEqual("Message", tracer.LogMessages[0].Message);
            }
        }

        /// <summary>
        /// Tests that verify that multiple threads can create logging tracers and that they get
        /// only the calls for their thread and that the lifetime of the tracer on other threads
        /// don't impact other threads.
        /// </summary>
        [Test]
        public void LoggingTracerMultiThreaded()
        {
            this.SetMessageHandlerImplementation();

            Task threadA = Task.Run(() => ThreadProc(1, 50));
            Task threadB = Task.Run(() => ThreadProc(10, 10));
            Task threadC = Task.Run(() => ThreadProc(11, 9));

            threadA.Wait();
            threadB.Wait();
            threadC.Wait();
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
                    Logger.Reset();
                }

                this.disposed = true;
            }
        }

        private static void ThreadProc(int delay, int count)
        {
            // Make sure that there are calls at times that a thread exists where the tracer isn't active.
            Assert.AreEqual(0, Device.GetInstalledCount());
            Thread.Sleep(delay);

            using (LoggingTracer tracer = new LoggingTracer())
            {
                for (int i = 0; i < count; ++i)
                {
                    Assert.AreEqual(0, Device.GetInstalledCount());
                    Thread.Sleep(delay);
                }

                Assert.AreEqual(count, tracer.LogMessages.Count);
            }

            Assert.AreEqual(0, Device.GetInstalledCount());
        }

        /// <summary>
        /// This sets up an implementation for the set debug message handler and for get installed
        /// count to enable testing. Every call to get the installed count should result in one
        /// message being generated.
        /// </summary>
        private void SetMessageHandlerImplementation()
        {
            this.nativeK4a.SetImplementation(@"

k4a_logging_message_cb_t *g_message_cb;

uint32_t k4a_device_get_installed_count(void)
{
    if(g_message_cb != NULL)
    {
        g_message_cb(NULL, K4A_LOG_LEVEL_WARNING, ""File"", 1234, ""Message"");
    }

    return 0;
}

k4a_result_t k4a_set_debug_message_handler(
    k4a_logging_message_cb_t *message_cb,
    void *message_cb_context,
    k4a_log_level_t min_level)
{
    STUB_ASSERT(message_cb != NULL);
    STUB_ASSERT(message_cb_context == NULL);
    STUB_ASSERT(min_level >= K4A_LOG_LEVEL_WARNING);

    g_message_cb = message_cb;

    return K4A_RESULT_SUCCEEDED;
}");
        }
    }
}
