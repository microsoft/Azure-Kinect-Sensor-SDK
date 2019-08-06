//------------------------------------------------------------------------------
// <copyright file="NativeFailureException.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Runtime.Serialization;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    /// <summary>
    /// Represents a failure from the native layers?
    /// </summary>
    [Serializable]
    public class NativeFailureException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="NativeFailureException"/> class.
        /// </summary>
        public NativeFailureException()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NativeFailureException"/> class.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="file"></param>
        /// <param name="line"></param>
        public NativeFailureException(string message, string file, int line)
            : base($"{file}:{line} {message}")
        {
            this.File = file;
            this.Line = line;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NativeFailureException"/> class.
        /// </summary>
        /// <param name="message">The error message that explains the reason for the exception.</param>
        /// <param name="innerException">The exception that is the cause of the current exception, or a null reference (Nothing in Visual Basic) if no inner exception is specified.</param>
        public NativeFailureException(string message, Exception inner)
            : base(message, inner)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="NativeFailureException"/> class.
        /// </summary>
        /// <param name="info">The <see cref="SerializationInfo"/> that holds the serialized object data about the exception being thrown.</param>
        /// <param name="context">The <see cref="StreamingContext"/>System.Runtime.Serialization.StreamingContext that contains contextual information about the source or destination.</param>
        protected NativeFailureException(SerializationInfo info, StreamingContext context)
            : base(info, context)
        {
        }

        /// <summary>
        /// Gets the file that the failure is coming from.
        /// </summary>
        public string File { get; }

        /// <summary>
        /// Gets the line that the failure is coming from.
        /// </summary>
        public int Line { get; }
    }
}
