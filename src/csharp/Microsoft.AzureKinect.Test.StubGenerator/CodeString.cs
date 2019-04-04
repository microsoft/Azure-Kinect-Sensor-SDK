using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect.Test.StubGenerator
{
    /// <summary>
    /// A string representation of C code
    /// </summary>
    /// <remarks>
    /// This class captures source and line metadata for the location where the string was assigned.
    /// 
    /// This information is used when compiling the code, or in debugging the code, to reference back to the
    /// source location where the string was assigned.
    /// </remarks>
    public class CodeString
    {
        private CodeString()
        {
        }

        /// <summary>
        /// Create a CodeString with some C code
        /// </summary>
        /// <param name="code"></param>
        public CodeString(string code)
        {
            AssignCode(code, 2);
        }

        private string code = "";

        // Performs the asignment and looks back frameDepth stack frames
        // for the origin information
        private void AssignCode(string code, int frameDepth)
        {
            var trace = new System.Diagnostics.StackTrace(true);
            var frame = trace.GetFrame(frameDepth);
            SourceLineNumber = frame.GetFileLineNumber();
            SourceFileName = frame.GetFileName();

            this.code = code;
        }

        public string Code
        {
            get
            {
                return code;
            }
            set
            {
                AssignCode(code, 2);
            }
        }

        public string SourceFileName { get; private set; } = "";
        public int SourceLineNumber { get; private set; } = 0;

        public static implicit operator string(CodeString s)
        {
            return s.Code;
        }

        public static implicit operator CodeString(string s)
        {
            CodeString c = new CodeString();
            c.AssignCode(s, 2);
            return c;
        }

        /// <summary>
        /// Returns a version of the code with #line data
        /// </summary>
        public string EmbdededLineData
        {
            get
            {
                StringBuilder source = new StringBuilder();

                source.AppendLine($"#line {SourceLineNumber} \"{SourceFileName.Replace("\\", "\\\\")}\"");
                source.AppendLine(Code);
                return source.ToString();
            }
        }

        public override string ToString()
        {
            return Code;
        }
    }
}
