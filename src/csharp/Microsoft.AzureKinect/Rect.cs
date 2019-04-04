using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.AzureKinect
{
    public struct Rect
    {
        public Rect(int x, int y, int width, int height)
        {
            X = x;
            Y = y;
            Width = width;
            Height = height;
        }
        
        public int X { get; set; }
        public int Y { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }

        public static Rect Empty { get; } = new Rect(0,0,0,0);
        public bool IsEmpty { get
            {
                return Width == 0 || Height == 0;
            }
        }

    }
}
