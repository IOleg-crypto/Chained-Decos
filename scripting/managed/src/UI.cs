using Coral.Managed.Interop;

namespace CHEngine
{
    public static class UI
    {
#pragma warning disable 0649
        internal static unsafe delegate*<NativeString, void> UI_Text_Ptr;
#pragma warning restore 0649

        public static unsafe void Text(string text) => UI_Text_Ptr(text);
    }
}
