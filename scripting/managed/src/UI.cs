using Coral.Managed.Interop;

namespace Chained
{
    /// <summary>UI helpers.</summary>
    public static class UI
    {
#pragma warning disable 0649
        internal static unsafe delegate* unmanaged<char*, void> UI_Text_Ptr;
        internal static unsafe delegate* unmanaged<char*, byte> UI_Button_Ptr;
#pragma warning restore 0649

        /// <summary>Draws UI text.</summary>
        public static unsafe void Text(string text)
        {
            if (text == null || UI_Text_Ptr == null) return;
            fixed (char* ptr = text) UI_Text_Ptr(ptr);
        }

        /// <summary>Renders a button. Returns true when clicked.</summary>
        public static unsafe bool Button(string label)
        {
            if (label == null || UI_Button_Ptr == null) return false;
            fixed (char* ptr = label) return UI_Button_Ptr(ptr) != 0;
        }
    }
}