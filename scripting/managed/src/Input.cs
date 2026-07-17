using System;

namespace Chained
{
    public static class Input
    {
        
        internal static unsafe delegate* unmanaged<int, uint> Input_IsKeyDown_Ptr;
        internal static unsafe delegate* unmanaged<int, uint> Input_IsKeyPressed_Ptr;
        internal static unsafe delegate* unmanaged<int, uint> Input_IsKeyReleased_Ptr;
        internal static unsafe delegate* unmanaged<int, uint> Input_IsMouseButtonDown_Ptr;
        internal static unsafe delegate* unmanaged<int, uint> Input_IsMouseButtonPressed_Ptr;
        internal static unsafe delegate* unmanaged<float> Input_GetMouseWheelMove_Ptr;
        internal static unsafe delegate* unmanaged<Vector3*, void> Input_GetMouseDelta_Ptr;
        
        public static unsafe bool IsKeyDown(Key keyCode) => Input_IsKeyDown_Ptr != null && Input_IsKeyDown_Ptr((int)keyCode) != 0;
        public static unsafe bool IsKeyPressed(Key keyCode) => Input_IsKeyPressed_Ptr != null && Input_IsKeyPressed_Ptr((int)keyCode) != 0;
        public static unsafe bool IsKeyReleased(Key keyCode) => Input_IsKeyReleased_Ptr != null && Input_IsKeyReleased_Ptr((int)keyCode) != 0;

        public static unsafe bool IsMouseButtonDown(MouseButton button) => Input_IsMouseButtonDown_Ptr != null && Input_IsMouseButtonDown_Ptr((int)button) != 0;
        public static unsafe bool IsMouseButtonPressed(MouseButton button) => Input_IsMouseButtonPressed_Ptr != null && Input_IsMouseButtonPressed_Ptr((int)button) != 0;

        public static unsafe float GetMouseWheelMove() => Input_GetMouseWheelMove_Ptr != null ? Input_GetMouseWheelMove_Ptr() : 0.0f;

        public static Vector3 MouseDelta
        {
            get
            {
                unsafe
                {
                    if (Input_GetMouseDelta_Ptr == null) return Vector3.Zero;
                    Vector3 delta;
                    Input_GetMouseDelta_Ptr(&delta);
                    return delta;
                }
            }
        }
    }
}