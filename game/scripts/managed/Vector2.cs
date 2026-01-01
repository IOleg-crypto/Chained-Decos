using System.Runtime.InteropServices;

namespace ChainedEngine
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector2
    {
        public float X, Y;

        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }

        public override string ToString() => $"({X}, {Y})";
    }
}
