//------------------------------------------------------------------------------
// <copyright file="LoggingTests.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using Microsoft.Azure.Kinect.Sensor.Test.StubGenerator;
using NUnit.Framework;

namespace Microsoft.Azure.Kinect.Sensor.UnitTests
{
    /// <summary>
    /// These tests validate the calling and marshaling for the logging and logging tracing
    /// features of the C# wrapper for the Azure Kinect SDK.
    /// </summary>
    [TestFixture]
    public class LoggingTests
    {
        private readonly StubbedModule nativeK4a;

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

            void EventHandler(object s, DebugMessageEventArgs e)
            {
                ++calledCount;
                Assert.IsNull(s);
                Assert.AreEqual(LogLevel.Warning, e.LogLevel);
                Assert.AreEqual("File", e.FileName);
                Assert.AreEqual(1234, e.Line);
                Assert.AreEqual("Message", e.Message);
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
                Assert.IsTrue(tracer.LogMessages[0].EndsWith("[Warning] File@1234: Message", StringComparison.Ordinal));
            }

            // Make sure logging getting called without a logging tracer doesn't cause a failure.
            Assert.AreEqual(0, Device.GetInstalledCount());

            // Make sure that we can instantiate another logging tracer.
            using (LoggingTracer tracer = new LoggingTracer())
            {
                _ = Assert.Throws(typeof(InvalidOperationException), () =>
                {
                    using (LoggingTracer trace = new LoggingTracer())
                    {
                    }
                });

                Assert.AreEqual(0, Device.GetInstalledCount());
                Assert.AreEqual(1, tracer.LogMessages.Count);
                Assert.IsTrue(tracer.LogMessages[0].EndsWith("[Warning] File@1234: Message", StringComparison.Ordinal));
            }
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
