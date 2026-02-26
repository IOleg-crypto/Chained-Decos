using Coral.Managed.Interop;

namespace CHEngine
{

public static class Input
{
#pragma warning disable 0649
    internal static unsafe delegate*<int, bool> Input_IsKeyDown_Ptr;
    internal static unsafe delegate*<int, bool> Input_IsKeyPressed_Ptr;
    internal static unsafe delegate*<int, bool> Input_IsKeyReleased_Ptr;
    internal static unsafe delegate*<int, bool> Input_IsMouseButtonDown_Ptr;
    internal static unsafe delegate*<int, bool> Input_IsMouseButtonPressed_Ptr;
    internal static unsafe delegate*<float> Input_GetMouseWheelMove_Ptr;
    internal static unsafe delegate*<Vector3*, void> Input_GetMouseDelta_Ptr;
#pragma warning restore 0649

    public static unsafe bool IsKeyDown(Key keyCode)           => Input_IsKeyDown_Ptr((int)keyCode);
    public static unsafe bool IsKeyPressed(Key keyCode)        => Input_IsKeyPressed_Ptr((int)keyCode);
    public static unsafe bool IsKeyReleased(Key keyCode)       => Input_IsKeyReleased_Ptr((int)keyCode);

    public static unsafe bool IsMouseButtonDown(MouseButton button)    => Input_IsMouseButtonDown_Ptr((int)button);
    public static unsafe bool IsMouseButtonPressed(MouseButton button) => Input_IsMouseButtonPressed_Ptr((int)button);

    public static unsafe float GetMouseWheelMove() => Input_GetMouseWheelMove_Ptr();

    public static Vector3 MouseDelta
    {
        get { unsafe { Vector3 d; Input_GetMouseDelta_Ptr(&d); return d; } }
    }
}

}

