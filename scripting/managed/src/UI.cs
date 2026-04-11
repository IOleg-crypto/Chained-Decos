using Coral.Managed.Interop;

namespace CHEngine
{
    /// <summary>UI helpers.</summary>
    public static class UI
    {
#pragma warning disable 0649
        internal static unsafe delegate*<NativeString, void> UI_Text_Ptr;
#pragma warning restore 0649

        /// <summary>Draws UI text.</summary>
        public static unsafe void Text(string text) => UI_Text_Ptr(text);
    }
}
