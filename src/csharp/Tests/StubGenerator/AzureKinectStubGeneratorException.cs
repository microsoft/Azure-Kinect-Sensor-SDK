//------------------------------------------------------------------------------
// <copyright file="AzureKinectStubGeneratorException.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.Serialization;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    /// <summary>
    /// Represents errors that occur when interactive with the Azure Kinect Sensor SDK.
    /// </summary>
    [Serializable]
    public class AzureKinectStubGeneratorException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStubGeneratorException"/> class.
        /// </summary>
        public AzureKinectStubGeneratorException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStubGeneratorException"/> class with a specified error message.
        /// </summary>
        /// <param name="message">The message that describes the error.</param>
        public AzureKinectStubGeneratorException(string message)
            : base(message)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStubGeneratorException"/> class with a specified error message and a reference to the inner exception that is the cause of this exception.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception, or a null reference (Nothing in Visual Basic) if no inner exception is specified.</param>
        public AzureKinectStubGeneratorException(string message, Exception innerException)
            : base(message, innerException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="AzureKinectStubGeneratorException"/> class with serialized data.
        /// </summary>
        /// <param name="info">The <see cref="SerializationInfo"/> that holds the serialized object data about the exception being thrown.</param>
        /// <param name="context">The <see cref="StreamingContext"/>System.Runtime.Serialization.StreamingContext that contains contextual information about the source or destination.</param>
        protected AzureKinectStubGeneratorException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }
    }
}
