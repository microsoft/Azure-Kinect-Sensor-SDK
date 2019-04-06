using NUnit.Framework;
using Microsoft.AzureKinect.Test.StubGenerator;
using Microsoft.AzureKinect;

namespace Microsoft.AzureKinect.UnitTests
{
   
    public class StructureTests
    {
        [Test]
        public void Float2()
        {
            Float2 f = new Float2(1, 2.0f);

            Assert.AreEqual(1, f.X);
            Assert.AreEqual(2.0f, f.Y);

            f.X = 3.0f;
            Assert.AreEqual(3.0f, f.X);
            f.Y = 4.0f;
            Assert.AreEqual(4.0f, f.Y);

            Float2 f2 = new Float2(3.0f, 4.0f);

            Assert.AreEqual(f, f2);

            Float2 f3 = new Float2(3.0f, 5.0f);
            Assert.AreNotEqual(f2, f3);
            
        }

        [Test]
        public void Float3()
        {
            Float3 f = new Float3(1, 2.0f, 3.0f);

            Assert.AreEqual(1, f.X);
            Assert.AreEqual(2.0f, f.Y);
            Assert.AreEqual(3.0f, f.Z);

            f.X = 3.0f;
            Assert.AreEqual(3.0f, f.X);
            f.Y = 4.0f;
            Assert.AreEqual(4.0f, f.Y);
            f.Z = 5.5f;
            Assert.AreEqual(5.5f, f.Z);

            Float3 f2 = new Float3(3.0f, 4.0f, 5.5f);

            Assert.AreEqual(f, f2);

            Float3 f3 = new Float3(3.0f, 5.0f, 6.0f);
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

        }

        [Test]
        public void Rect()
        {
            Rect r1 = new Rect(1, 2, 3, 4);

            Assert.AreEqual(1, r1.X);
            Assert.AreEqual(2, r1.Y);
            Assert.AreEqual(3, r1.Width);
            Assert.AreEqual(4, r1.Height);

            r1.X = 10;
            r1.Y = 11;
            r1.Width = 12;
            r1.Height = 13;

            Assert.AreEqual(10, r1.X);
            Assert.AreEqual(11, r1.Y);
            Assert.AreEqual(12, r1.Width);
            Assert.AreEqual(13, r1.Height);
        }
    }
}
