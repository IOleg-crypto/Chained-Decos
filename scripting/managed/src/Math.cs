using System.Runtime.InteropServices;

namespace CHEngine
{

// ── Math ─────────────────────────────────────────────────────────────────────

/// <summary>3D vector shared with native code.</summary>
[StructLayout(LayoutKind.Sequential)]
public struct Vector3
{
    public float X, Y, Z;

    /// <summary>Creates a vector from X, Y, and Z.</summary>
    public Vector3(float x, float y, float z) { X = x; Y = y; Z = z; }

    public static readonly Vector3 Zero  = new Vector3(0, 0, 0);
    public static readonly Vector3 One   = new Vector3(1, 1, 1);
    public static readonly Vector3 Up    = new Vector3(0, 1, 0);

    public static Vector3 operator +(Vector3 left, Vector3 right) => new Vector3(left.X + right.X, left.Y + right.Y, left.Z + right.Z);
    public static Vector3 operator -(Vector3 left, Vector3 right) => new Vector3(left.X - right.X, left.Y - right.Y, left.Z - right.Z);
    public static Vector3 operator -(Vector3 vector)              => new Vector3(-vector.X, -vector.Y, -vector.Z);
    public static Vector3 operator *(Vector3 vector, float scalar) => new Vector3(vector.X * scalar, vector.Y * scalar, vector.Z * scalar);
    public static Vector3 operator *(float scalar, Vector3 vector) => new Vector3(scalar * vector.X, scalar * vector.Y, scalar * vector.Z);
    public static Vector3 operator /(Vector3 vector, float scalar) => new Vector3(vector.X / scalar, vector.Y / scalar, vector.Z / scalar);

    public float LengthSquared() => X * X + Y * Y + Z * Z;
    public float Length()        => (float)System.Math.Sqrt(LengthSquared());

    /// <summary>Returns a normalized copy, or zero for tiny inputs.</summary>
    public static Vector3 Normalize(Vector3 vector)
    {
        float length = vector.Length();
        return length > 0.00001f ? vector / length : Zero;
    }

    /// <summary>Dot product.</summary>
    public static float Dot(Vector3 left, Vector3 right) => left.X * right.X + left.Y * right.Y + left.Z * right.Z;

    /// <summary>Cross product.</summary>
    public static Vector3 Cross(Vector3 left, Vector3 right) => new Vector3(
        left.Y * right.Z - left.Z * right.Y,
        left.Z * right.X - left.X * right.Z,
        left.X * right.Y - left.Y * right.X);

    /// <summary>Linear interpolation.</summary>
    public static float Lerp(float start, float end, float factor) => start + (end - start) * factor;

    public override string ToString() => $"({X:F2}, {Y:F2}, {Z:F2})";
}

/// <summary>4D vector shared with native code (useful for colors).</summary>
[StructLayout(LayoutKind.Sequential)]
public struct Vector4
{
    public float X, Y, Z, W;

    public Vector4(float x, float y, float z, float w) { X = x; Y = y; Z = z; W = w; }
    public Vector4(float value) { X = Y = Z = W = value; }

    public static readonly Vector4 White = new Vector4(1, 1, 1, 1);
    public static readonly Vector4 Black = new Vector4(0, 0, 0, 1);
}

/// <summary>Scalar math helpers.</summary>
public static class Mathf
{
    public const float PI      = (float)System.Math.PI;
    public const float Deg2Rad = PI / 180.0f;
    public const float Rad2Deg = 180.0f / PI;

    public static float Clamp(float value, float min, float max)
        => value < min ? min : (value > max ? max : value);
    public static float Abs(float value) => value < 0 ? -value : value;
    public static float Sin(float angle) => (float)System.Math.Sin(angle);
    public static float Cos(float angle) => (float)System.Math.Cos(angle);
    public static float Atan2(float y, float x) => (float)System.Math.Atan2(y, x);
    public static float Sqrt(float value) => (float)System.Math.Sqrt(value);
    public static float Lerp(float start, float end, float factor) => start + (end - start) * factor;
}

// ── Key / MouseButton enums ───────────────────────────────────────────────────

/// <summary>Keyboard keys exposed to managed scripts.</summary>
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

/// <summary>Mouse buttons exposed to managed scripts.</summary>
public enum MouseButton : int
{
    Left   = 0,
    Right  = 1,
    Middle = 2
}

} // namespace CHEngine
