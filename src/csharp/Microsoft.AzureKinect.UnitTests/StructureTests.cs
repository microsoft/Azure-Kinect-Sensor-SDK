using System.Numerics;
using NUnit.Framework;
using Microsoft.AzureKinect.Test.StubGenerator;
using Microsoft.AzureKinect;

namespace Microsoft.AzureKinect.UnitTests
{
   
    public class StructureTests
    {
        [Test]
        public void Vector2()
        {
            Vector2 f = new Vector2(1, 2.0f);

            Assert.AreEqual(1, f.X);
            Assert.AreEqual(2.0f, f.Y);

            f.X = 3.0f;
            Assert.AreEqual(3.0f, f.X);
            f.Y = 4.0f;
            Assert.AreEqual(4.0f, f.Y);

            Vector2 f2 = new Vector2(3.0f, 4.0f);

            Assert.AreEqual(f, f2);

            Vector2 f3 = new Vector2(3.0f, 5.0f);
            Assert.AreNotEqual(f2, f3);
            
        }

        [Test]
        public void Vector3()
        {
            Vector3 f = new Vector3(1, 2.0f, 3.0f);

            Assert.AreEqual(1, f.X);
            Assert.AreEqual(2.0f, f.Y);
            Assert.AreEqual(3.0f, f.Z);

            f.X = 3.0f;
            Assert.AreEqual(3.0f, f.X);
            f.Y = 4.0f;
            Assert.AreEqual(4.0f, f.Y);
            f.Z = 5.5f;
            Assert.AreEqual(5.5f, f.Z);

            Vector3 f2 = new Vector3(3.0f, 4.0f, 5.5f);

            Assert.AreEqual(f, f2);

            Vector3 f3 = new Vector3(3.0f, 5.0f, 6.0f);
            Assert.AreNotEqual(f2, f3);

        }

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
