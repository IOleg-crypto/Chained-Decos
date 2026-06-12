using Coral.Managed.Interop;

namespace Chained
{

/// <summary>Input helpers.</summary>
public static class Input
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<int, bool> Input_IsKeyDown_Ptr;
    internal static unsafe delegate* unmanaged<int, bool> Input_IsKeyPressed_Ptr;
    internal static unsafe delegate* unmanaged<int, bool> Input_IsKeyReleased_Ptr;
    internal static unsafe delegate* unmanaged<int, bool> Input_IsMouseButtonDown_Ptr;
    internal static unsafe delegate* unmanaged<int, bool> Input_IsMouseButtonPressed_Ptr;
    internal static unsafe delegate* unmanaged<float> Input_GetMouseWheelMove_Ptr;
    internal static unsafe delegate* unmanaged<Vector3*, void> Input_GetMouseDelta_Ptr;
#pragma warning restore 0649

    /// <summary>True while the key is held.</summary>
    public static unsafe bool IsKeyDown(Key keyCode) => Input_IsKeyDown_Ptr((int)keyCode);
    /// <summary>True on the press frame.</summary>
    public static unsafe bool IsKeyPressed(Key keyCode) => Input_IsKeyPressed_Ptr((int)keyCode);
    /// <summary>True on the release frame.</summary>
    public static unsafe bool IsKeyReleased(Key keyCode) => Input_IsKeyReleased_Ptr((int)keyCode);

    /// <summary>True while the mouse button is held.</summary>
    public static unsafe bool IsMouseButtonDown(MouseButton button) => Input_IsMouseButtonDown_Ptr((int)button);
    /// <summary>True on the mouse press frame.</summary>
    public static unsafe bool IsMouseButtonPressed(MouseButton button) => Input_IsMouseButtonPressed_Ptr((int)button);

    /// <summary>Mouse wheel delta for this frame.</summary>
    public static unsafe float GetMouseWheelMove() => Input_GetMouseWheelMove_Ptr();

    /// <summary>Mouse movement delta for this frame.</summary>
    public static Vector3 MouseDelta
    {
        get { unsafe { Vector3 delta; Input_GetMouseDelta_Ptr(&delta); return delta; } }
    }
}

}

