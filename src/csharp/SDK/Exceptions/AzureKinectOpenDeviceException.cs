//------------------------------------------------------------------------------
// <copyright file="AzureKinectOpenDeviceException.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Runtime.Serialization;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents errors that occur when opening a device with the Azure Kinect Sensor SDK.
    /// </summary>
    [Serializable]
    public class AzureKinectOpenDeviceException : AzureKinectException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class.
        /// </summary>
        public AzureKinectOpenDeviceException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class
        /// with a specified error message.
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        public AzureKinectOpenDeviceException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class
        /// with a specified error message and a reference to the inner exception that is the
        /// cause of this exception.
        /// </summary>
        /// <param name="message">
        /// The error message that explains the reason for the exception.
        /// </param>
        /// <param name="innerException">
        /// The exception that is the cause of the current exception, or a null reference
        /// (Nothing in Visual Basic) if no inner exception is specified.
        /// </param>
        public AzureKinectOpenDeviceException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class
        /// with serialized data.
        /// </summary>
        /// <param name="info">
        /// The <see cref="SerializationInfo"/> that holds the serialized object data about the
        /// exception being thrown.</param>
        /// <param name="context">
        /// The <see cref="StreamingContext"/>System.Runtime.Serialization.StreamingContext that
        /// contains contextual information about the source or destination.
        /// </param>
        protected AzureKinectOpenDeviceException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectOpenDeviceException"/> class
        /// with a specified error message.
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        /// <param name="logMessages">
        /// The log messages that happened during the function call that generated this error.
        /// </param>
        protected AzureKinectOpenDeviceException(string message, ICollection<LogMessage> logMessages)
            : base(message, logMessages)
        {
        }

        /// <summary>
        /// Throws an <see cref="AzureKinectOpenDeviceException"/> if the result of the function
        /// is not a success.
        /// </summary>
        /// <param name="function">The native function to call.</param>
        /// <typeparam name="T">The type of result to expect from the function call.</typeparam>
        internal static new void ThrowIfNotSuccess<T>(Func<T> function)
            where T : System.Enum
        {
            using (LoggingTracer tracer = new LoggingTracer())
            {
                T result = function();
                if (!AzureKinectException.IsSuccess(result))
                {
                    throw new AzureKinectOpenDeviceException($"result = {result}", tracer.LogMessages);
                }
            }
        }

        /// <summary>
        /// Throws an <see cref="AzureKinectOpenDeviceException"/> if the result of the function
        /// is not a success.
        /// </summary>
        /// <param name="tracer">The tracer is that is capturing logging messages.</param>
        /// <param name="result">The result native function to call.</param>
        /// <typeparam name="T">The type of result to expect from the function call.</typeparam>
        internal static new void ThrowIfNotSuccess<T>(LoggingTracer tracer, T result)
            where T : System.Enum
        {
            if (!AzureKinectException.IsSuccess(result))
            {
                throw new AzureKinectOpenDeviceException($"result = {result}", tracer.LogMessages);
            }
        }
    }
}
