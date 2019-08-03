//------------------------------------------------------------------------------
// <copyright file="NativeCaller.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Enables calling native Azure Kinect Sensor SDK calls with capturing of the log messages
    /// that happened during that call.
    /// </summary>
    internal static class NativeCaller
    {
        /// <summary>
        /// Enables logging and then calls the supplied function and returns an exception if the
        /// call does not return a success.
        /// </summary>
        /// <typeparam name="T">The type of result to expect from the function call.</typeparam>
        /// <param name="function">The function call to capture logging for.</param>
        /// <param name="exceptionFunc">
        /// A function to generate the exception if the call fails.
        /// </param>
        /// <returns>
        /// The exception generated from the native function call if an error occurred; otherwise
        /// null.
        /// </returns>
        internal static AzureKinectException Call<T>(Func<T> function, Func<NativeCallResults<T>, AzureKinectException> exceptionFunc)
            where T : System.Enum
        {
            using (LoggingTracer tracer = new LoggingTracer())
            {
                NativeCallResults<T> callResult = new NativeCallResults<T>(function(), tracer.LogMessages);

                return IsFailure(callResult.Result) ? exceptionFunc(callResult) : null;
            }
        }

        /// <summary>
        /// Completes the logging for a native function call that happened on this thread and
        /// returns an exception if the call did not return a success.
        /// </summary>
        /// <typeparam name="T">The type of result to expect from the function call.</typeparam>
        /// <param name="tracer">The tracer is that is capturing logging messages.</param>
        /// <param name="result">The function call to capture logging for.</param>
        /// <param name="exceptionFunc">
        /// A function to generate the exception if the call fails.
        /// </param>
        /// <returns>
        /// The exception generated from the native function call if an error occurred; otherwise
        /// null.
        /// </returns>
        internal static AzureKinectException EndCall<T>(LoggingTracer tracer, T result, Func<NativeCallResults<T>, AzureKinectException> exceptionFunc)
            where T : System.Enum
        {
            NativeCallResults<T> callResult = new NativeCallResults<T>(result, tracer.LogMessages);

            return IsFailure(callResult.Result) ? exceptionFunc(callResult) : null;
        }

        private static bool IsFailure<T>(T result)
            where T : Enum
        {
            switch (result)
            {
                case NativeMethods.k4a_result_t k4a_result:
                    return IsFailure(k4a_result);

                case NativeMethods.k4a_wait_result_t k4a_result:
                    return IsFailure(k4a_result);

                case NativeMethods.k4a_buffer_result_t k4a_result:
                    return IsFailure(k4a_result);

                default:
                    throw new ArgumentException("Result is not of a recognized result type.", nameof(result));
            }
        }

        private static bool IsFailure(NativeMethods.k4a_result_t result)
        {
            return result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED;
        }

        private static bool IsFailure(NativeMethods.k4a_wait_result_t result)
        {
            return result != NativeMethods.k4a_wait_result_t.K4A_WAIT_RESULT_SUCCEEDED;
        }

        private static bool IsFailure(NativeMethods.k4a_buffer_result_t result)
        {
            return result != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_SUCCEEDED;
        }

        /// <summary>
        /// Represents the results of a call to a native Azure Kinect Sensor SDK function with
        /// logging.
        /// </summary>
        /// <typeparam name="T">The result to expect from the function call.</typeparam>
        internal class NativeCallResults<T>
        {
            /// <summary>
            /// Initializes a new instance of the <see cref="NativeCallResults{T}"/> class.
            /// </summary>
            /// <param name="result">The results of the function call.</param>
            /// <param name="logMessages">
            /// The log messages that were generated while the function ran.
            /// </param>
            public NativeCallResults(T result, ICollection<string> logMessages)
            {
                this.Result = result;
                this.LogMessages = logMessages;
            }

            /// <summary>
            /// Gets the result of the function call.
            /// </summary>
            public T Result { get; }

            /// <summary>
            /// Gets the log messages that were generated while the function ran.
            /// </summary>
            public ICollection<string> LogMessages { get; }
        }
    }
}
