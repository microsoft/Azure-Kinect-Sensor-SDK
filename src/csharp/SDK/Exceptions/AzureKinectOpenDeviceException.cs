//------------------------------------------------------------------------------
// <copyright file="AzureKinectOpenDeviceException.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.Serialization;

namespace Microsoft.Azure.Kinect.Sensor.Exceptions
{
    /// <summary>
    /// Represents errors that occur when opening a device with the Azure Kinect Sensor SDK.
    /// </summary>
    public class AzureKinectOpenDeviceException : AzureKinectException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class.
        /// </summary>
        public AzureKinectOpenDeviceException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        public AzureKinectOpenDeviceException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception, or a null reference (Nothing in Visual Basic) if no inner exception is specified.</param>
        public AzureKinectOpenDeviceException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class with serialized data.
        /// </summary>
        /// <param name="info">The <see cref="SerializationInfo"/> that holds the serialized object data about the exception being thrown.</param>
        /// <param name="context">The <see cref="StreamingContext"/>System.Runtime.Serialization.StreamingContext that contains contextual information about the source or destination.</param>
        protected AzureKinectOpenDeviceException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// Throws an <see cref="AzureKinectOpenDeviceException"/> if the result is not <see cref="NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED"/>.
        /// </summary>
        /// <param name="result">The result to check.</param>
        internal static void ThrowIfNotSuccess(NativeMethods.k4a_result_t result)
        {
            if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
            {
                throw new AzureKinectOpenDeviceException($"result = {result}");
            }
        }
    }
}
