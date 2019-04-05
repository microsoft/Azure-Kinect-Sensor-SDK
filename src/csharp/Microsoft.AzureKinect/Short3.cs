using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    public struct Short3
    {
        public Short3(Int16 X, Int16 Y, Int16 Z)
        {
            this.X = X;
            this.Y = Y;
            this.Z = Z;
        }
               
        public Int16 X { get; set; }

        public Int16 Y { get; set; }

        public Int16 Z { get; set; }

        public Int16 this[int i]
        {
            get
            {
                switch (i)
                {
                    case 0:
                        return X;
                    case 1:
                        return Y;
                    case 2:
                        return Z;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
            set
            {
                switch (i)
                {
                    case 0:
                        X = value;
                        return;
                    case 1:
                        Y = value;
                        return;
                    case 2:
                        Z = value;
                        return;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }
    }
}

