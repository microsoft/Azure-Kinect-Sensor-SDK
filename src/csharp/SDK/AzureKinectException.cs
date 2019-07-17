//------------------------------------------------------------------------------
// <copyright file="AzureKinectException.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.Serialization;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents errors that occur when interactive with the Azure Kinect Sensor SDK.
    /// </summary>
    [Serializable]
    public class AzureKinectException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectException"/> class.
        /// </summary>
        public AzureKinectException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        public AzureKinectException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectException"/> class with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception, or a null reference (Nothing in Visual Basic) if no inner exception is specified.</param>
        public AzureKinectException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectException"/> class with serialized data.
        /// </summary>
        /// <param name="info">The <see cref="SerializationInfo"/> that holds the serialized object data about the exception being thrown.</param>
        /// <param name="context">The <see cref="StreamingContext"/>System.Runtime.Serialization.StreamingContext that contains contextual information about the source or destination.</param>
        protected AzureKinectException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// Throws an <see cref="AzureKinectException"/> if the result is not <see cref="NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED"/>.
        /// </summary>
        /// <param name="result">The result to check.</param>
        internal static void ThrowIfNotSuccess(NativeMethods.k4a_result_t result)
        {
            if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
            {
                throw new AzureKinectException($"result = {result}");
            }
        }

        /// <summary>
        /// Throws an <see cref="AzureKinectException"/> if the result is not <see cref="NativeMethods.k4a_wait_result_t.K4A_WAIT_RESULT_SUCCEEDED"/>.
        /// </summary>
        /// <param name="result">The result to check.</param>
        internal static void ThrowIfNotSuccess(NativeMethods.k4a_wait_result_t result)
        {
            if (result != NativeMethods.k4a_wait_result_t.K4A_WAIT_RESULT_SUCCEEDED)
            {
                throw new AzureKinectException($"result = {result}");
            }
        }

        /// <summary>
        /// Throws an <see cref="AzureKinectException"/> if the result is not <see cref="NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_SUCCEEDED"/>.
        /// </summary>
        /// <param name="result">The result to check.</param>
        internal static void ThrowIfNotSuccess(NativeMethods.k4a_buffer_result_t result)
        {
            if (result != NativeMethods.k4a_buffer_result_t.K4A_BUFFER_RESULT_SUCCEEDED)
            {
                throw new AzureKinectException($"result = {result}");
            }
        }
    }
}
