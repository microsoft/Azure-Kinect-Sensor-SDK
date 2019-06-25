using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{

    [Serializable]
    public class NativeFailureException : Exception
    {
        public string File { get;  }
        public int Line { get; }

        public NativeFailureException() { }
        public NativeFailureException(string message, string file, int line) : base($"{file}:{line} {message}") {
            this.File = file;
            this.Line = line;
        }
        public NativeFailureException(string message, Exception inner) : base(message, inner) { }
        protected NativeFailureException(
          System.Runtime.Serialization.SerializationInfo info,
          System.Runtime.Serialization.StreamingContext context) : base(info, context) { }
    }
}
