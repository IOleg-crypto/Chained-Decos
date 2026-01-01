using System;
using System.Runtime.InteropServices;

namespace ChainedEngine
{
    public static class ImGui
    {
        public static void Text(string text) => InternalCalls.ImGui_Text(Marshal.StringToCoTaskMemUni(text));
        
        public static bool Button(string label) => InternalCalls.ImGui_Button(Marshal.StringToCoTaskMemUni(label));
        
        public static bool Checkbox(string label, ref bool value) => InternalCalls.ImGui_Checkbox(Marshal.StringToCoTaskMemUni(label), ref value);

        public static bool SliderFloat(string label, ref float value, float min, float max) 
            => InternalCalls.ImGui_SliderFloat(Marshal.StringToCoTaskMemUni(label), ref value, min, max);

        public static void Begin(string name) => InternalCalls.ImGui_Begin(Marshal.StringToCoTaskMemUni(name));
        public static void End() => InternalCalls.ImGui_End();
        
        public static void SameLine() => InternalCalls.ImGui_SameLine();
        public static void Separator() => InternalCalls.ImGui_Separator();
    }
}
