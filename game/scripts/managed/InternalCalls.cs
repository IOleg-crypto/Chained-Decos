using System;
using System.Runtime.InteropServices;

namespace ChainedEngine
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void LogInfoFn(IntPtr message);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void Vector3Fn(uint entityID, out Vector3 pos);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void Vector3RefFn(uint entityID, ref Vector3 pos);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void Vector2Fn(uint entityID, out Vector2 pos);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void Vector2RefFn(uint entityID, ref Vector2 pos);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void ByteFn(uint entityID, byte val);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate byte GetByteFn(uint entityID);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void BoolFn(uint entityID, bool val);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate bool GetBoolFn(uint entityID);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void FloatFn(uint entityID, float val);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate float GetFloatFn(uint entityID);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void StringFn(uint entityID, IntPtr str);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate IntPtr GetStringFn(uint entityID);

    // ImGui Delegates
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void ImGuiTextFn(IntPtr text);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate bool ImGuiButtonFn(IntPtr label);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate bool ImGuiCheckboxFn(IntPtr label, ref bool value);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate bool ImGuiSliderFloatFn(IntPtr label, ref float value, float min, float max);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void ImGuiVoidFn();

    public static class InternalCalls
    {
        public static LogInfoFn? LogInfo;
        public static LogInfoFn? LogWarning;
        public static LogInfoFn? LogError;

        // Transform
        public static Vector3Fn? Transform_GetPosition;
        public static Vector3RefFn? Transform_SetPosition;

        // RectTransform
        public static Vector2Fn? RectTransform_GetPosition;
        public static Vector2RefFn? RectTransform_SetPosition;
        public static Vector2Fn? RectTransform_GetSize;
        public static Vector2RefFn? RectTransform_SetSize;
        public static GetByteFn? RectTransform_GetAnchor;
        public static ByteFn? RectTransform_SetAnchor;
        public static GetBoolFn? RectTransform_IsActive;
        public static BoolFn? RectTransform_SetActive;

        // UI Components
        public static GetStringFn? UIButton_GetText;
        public static StringFn? UIButton_SetText;
        public static GetBoolFn? UIButton_IsClicked;
        public static GetStringFn? UIText_GetText;
        public static StringFn? UIText_SetText;
        public static GetFloatFn? UIText_GetFontSize;
        public static FloatFn? UIText_SetFontSize;

        // Entity
        public static Func<uint, string, bool>? Entity_HasComponent;

        // ImGui
        public static ImGuiTextFn? ImGui_Text;
        public static ImGuiButtonFn? ImGui_Button;
        public static ImGuiCheckboxFn? ImGui_Checkbox;
        public static ImGuiSliderFloatFn? ImGui_SliderFloat;
        public static ImGuiTextFn? ImGui_Begin;
        public static ImGuiVoidFn? ImGui_End;
        public static ImGuiVoidFn? ImGui_SameLine;
        public static ImGuiVoidFn? ImGui_Separator;

        [UnmanagedCallersOnly]
        public static void Initialize(IntPtr logInfo, IntPtr logWarn, IntPtr logError, IntPtr getPos, IntPtr setPos,
                                      IntPtr rectGetPos, IntPtr rectSetPos, IntPtr rectGetSize, IntPtr rectSetSize)
        {
            LogInfo = Marshal.GetDelegateForFunctionPointer<LogInfoFn>(logInfo);
            LogWarning = Marshal.GetDelegateForFunctionPointer<LogInfoFn>(logWarn);
            LogError = Marshal.GetDelegateForFunctionPointer<LogInfoFn>(logError);
            Transform_GetPosition = Marshal.GetDelegateForFunctionPointer<Vector3Fn>(getPos);
            Transform_SetPosition = Marshal.GetDelegateForFunctionPointer<Vector3RefFn>(setPos);
            
            RectTransform_GetPosition = Marshal.GetDelegateForFunctionPointer<Vector2Fn>(rectGetPos);
            RectTransform_SetPosition = Marshal.GetDelegateForFunctionPointer<Vector2RefFn>(rectSetPos);
            RectTransform_GetSize = Marshal.GetDelegateForFunctionPointer<Vector2Fn>(rectGetSize);
            RectTransform_SetSize = Marshal.GetDelegateForFunctionPointer<Vector2RefFn>(rectSetSize);

            Log.Info("C# Scripting System Initialized with UI support!");
        }
    }
}
