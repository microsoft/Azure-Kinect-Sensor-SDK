﻿//------------------------------------------------------------------------------
// <copyright file="AzureKinectStartCamerasException.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.Serialization;

namespace Microsoft.Azure.Kinect.Sensor
{
    /// <summary>
    /// Represents errors that occur when attempting to start the cameras on a device with the Azure Kinect Sensor SDK.
    /// </summary>
    public class AzureKinectStartCamerasException : AzureKinectException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStartCamerasException"/> class.
        /// </summary>
        public AzureKinectStartCamerasException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStartCamerasException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        public AzureKinectStartCamerasException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStartCamerasException"/> class with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception, or a null reference (Nothing in Visual Basic) if no inner exception is specified.</param>
        public AzureKinectStartCamerasException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStartCamerasException"/> class with serialized data.
        /// </summary>
        /// <param name="info">The <see cref="SerializationInfo"/> that holds the serialized object data about the exception being thrown.</param>
        /// <param name="context">The <see cref="StreamingContext"/>System.Runtime.Serialization.StreamingContext that contains contextual information about the source or destination.</param>
        protected AzureKinectStartCamerasException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// Throws an <see cref="AzureKinectStartCamerasException"/> if the result is not <see cref="NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED"/>.
        /// </summary>
        /// <param name="result">The result to check.</param>
        internal static void ThrowIfNotSuccess(NativeMethods.k4a_result_t result)
        {
            if (result != NativeMethods.k4a_result_t.K4A_RESULT_SUCCEEDED)
            {
                throw new AzureKinectStartCamerasException($"result = {result}");
            }
        }
    }
}
