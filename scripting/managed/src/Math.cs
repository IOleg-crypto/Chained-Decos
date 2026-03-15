using System.Runtime.InteropServices;

namespace CHEngine
{

// ── Math ─────────────────────────────────────────────────────────────────────

[StructLayout(LayoutKind.Sequential)]
public struct Vector3
{
    public float X, Y, Z;

    public Vector3(float x, float y, float z) { X = x; Y = y; Z = z; }

    public static readonly Vector3 Zero  = new Vector3(0, 0, 0);
    public static readonly Vector3 One   = new Vector3(1, 1, 1);
    public static readonly Vector3 Up    = new Vector3(0, 1, 0);

    public static Vector3 operator +(Vector3 a, Vector3 b) => new Vector3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
    public static Vector3 operator -(Vector3 a, Vector3 b) => new Vector3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
    public static Vector3 operator -(Vector3 a)            => new Vector3(-a.X, -a.Y, -a.Z);
    public static Vector3 operator *(Vector3 a, float b)   => new Vector3(a.X * b, a.Y * b, a.Z * b);
    public static Vector3 operator *(float a, Vector3 b)   => new Vector3(a * b.X, a * b.Y, a * b.Z);
    public static Vector3 operator /(Vector3 a, float b)   => new Vector3(a.X / b, a.Y / b, a.Z / b);

    public float LengthSquared() => X * X + Y * Y + Z * Z;
    public float Length()        => (float)System.Math.Sqrt(LengthSquared());

    public static Vector3 Normalize(Vector3 v)
    {
        float len = v.Length();
        return len > 0.00001f ? v / len : Zero;
    }

    public static float Dot(Vector3 a, Vector3 b) => a.X * b.X + a.Y * b.Y + a.Z * b.Z;

    public static Vector3 Cross(Vector3 a, Vector3 b) => new Vector3(
        a.Y * b.Z - a.Z * b.Y,
        a.Z * b.X - a.X * b.Z,
        a.X * b.Y - a.Y * b.X);

    public static float Lerp(float a, float b, float t) => a + (b - a) * t;

    public override string ToString() => $"({X:F2}, {Y:F2}, {Z:F2})";
}

public static class Mathf
{
    public const float PI      = (float)System.Math.PI;
    public const float Deg2Rad = PI / 180.0f;
    public const float Rad2Deg = 180.0f / PI;

    public static float Clamp(float v, float min, float max)
        => v < min ? min : (v > max ? max : v);
    public static float Abs(float v) => v < 0 ? -v : v;
    public static float Sin(float v) => (float)System.Math.Sin(v);
    public static float Cos(float v) => (float)System.Math.Cos(v);
    public static float Atan2(float y, float x) => (float)System.Math.Atan2(y, x);
    public static float Sqrt(float v) => (float)System.Math.Sqrt(v);
    public static float Lerp(float a, float b, float t) => a + (b - a) * t;
}

// ── Key / MouseButton enums ───────────────────────────────────────────────────

public enum Key : int
{
    A = 65,  B = 66,  C = 67,
    D = 68,  E = 69,  F = 70,
    G = 71,  H = 72,  I = 73,
    J = 74,  K = 75,  L = 76,
    M = 77,  N = 78,  O = 79,
    P = 80,  Q = 81,  R = 82,
    S = 83,  T = 84,  U = 85,
    V = 86,  W = 87,  X = 88,
    Y = 89,  Z = 90,

    Space      = 32,
    Enter      = 257,
    Escape     = 256,
    Backspace  = 259,
    Tab        = 258,
    LeftShift  = 340,
    RightShift = 344,
    LeftCtrl   = 341,
    LeftAlt    = 342,

    F1  = 290, F2  = 291, F3  = 292,
    F4  = 293, F5  = 294, F6  = 295,
    F7  = 296, F8  = 297, F9  = 298,
    F10 = 299, F11 = 300, F12 = 301,

    Up    = 265, Down  = 264,
    Left  = 263, Right = 262,

    D0 = 48, D1 = 49, D2 = 50,
    D3 = 51, D4 = 52, D5 = 53,
    D6 = 54, D7 = 55, D8 = 56,
    D9 = 57
}

public enum MouseButton : int
{
    Left   = 0,
    Right  = 1,
    Middle = 2
}

} // namespace CHEngine
