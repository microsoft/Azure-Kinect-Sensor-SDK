// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
using NUnit.Framework;

namespace Microsoft.Azure.Kinect.Sensor.UnitTests
{
    public class StructureTests
    {
        [Test]
        public void Short3()
        {
            Short3 s1 = new Short3(1, 2, 3);

            Assert.AreEqual(1, s1.X);
            Assert.AreEqual(2, s1.Y);
            Assert.AreEqual(3, s1.Z);

            s1.X = 3;
            Assert.AreEqual(3, s1.X);
            s1.Y = 4;
            Assert.AreEqual(4, s1.Y);
            s1.Z = 5;
            Assert.AreEqual(5, s1.Z);

            Short3 s2 = new Short3(3, 4, 5);

            Assert.AreEqual(s1, s2);

            Short3 s3 = new Short3(3, 5, 6);
            Assert.AreNotEqual(s2, s3);

        }

        [Test]
        public void BGRA()
        {
            BGRA b1 = new BGRA(1, 2, 3, 4);
            BGRA b2 = new BGRA(1, 2, 3);

            Assert.AreEqual(1, b1.B);
            Assert.AreEqual(2, b1.G);
            Assert.AreEqual(3, b1.R);
            Assert.AreEqual(4, b1.A);

            Assert.AreEqual(0x04030201, b1.Value);

            Assert.AreNotEqual(b1, b2);
            b2.A = 4;
            Assert.AreEqual(b1, b2);

            Assert.AreEqual(true, b1.Equals(b2));

        }
    }
}
