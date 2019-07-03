// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using System.Linq;
using System.Runtime.Serialization;
using System.Text;

namespace Microsoft.Azure.Kinect.Sensor.Test.StubGenerator
{
    /// <summary>
    /// Hashes an arbitrary object.
    /// </summary>
    internal class Hash
    {
        /// <summary>
        /// Serialize an object to XML and compute its hash.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="object"></param>
        /// <returns></returns>
        private static byte[] HashSerializedObject<T>(T @object)
        {
            DataContractSerializer xs = new DataContractSerializer(typeof(T));
            using (System.IO.MemoryStream memoryStream = new System.IO.MemoryStream())
            {
                xs.WriteObject(memoryStream, @object);
                return HashBytes(memoryStream.GetBuffer());
            }
        }

        private static byte[] HashBytes(byte[] input)
        {
            var md5 = System.Security.Cryptography.MD5.Create();
            return md5.ComputeHash(input);
        }

        private Hash(byte[] hash)
        {
            this.hash = hash;
        }

        public static Hash GetHash<T>(T @object)
        {
            return new Hash(HashSerializedObject(@object));
        }

        public static Hash GetHash(CompilerOptions options)
        {
            return new Hash(options.GetOptionsHash());
        }

        public static Hash operator +(Hash left, Hash right)
        {
            return new Hash(HashBytes(left.hash.Concat(right.hash).ToArray()));
        }

        public static implicit operator string(Hash h)
        {
            return h.ToString();
        }

        private readonly byte[] hash;

        public override string ToString()
        {
            StringBuilder hashString = new StringBuilder();
            foreach (byte b in hash)
            {
                hashString.Append(b.ToString("X2"));
            }
            return hashString.ToString();
        }

        public override int GetHashCode()
        {
            return this.ToString().GetHashCode();
        }

        public override bool Equals(object obj)
        {
            if (obj is string)
            {
                return this.ToString().Equals((string)obj);
            }
            else if (obj is Hash)
            {
                return this.ToString().Equals(obj.ToString());
            }
            else
            {
                return false;
            }
        }
    }
}
