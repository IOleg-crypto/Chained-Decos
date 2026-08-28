using System;
using System.Runtime.InteropServices;
using Coral.Managed.Interop;

namespace Chained
{

// ── UI Controls ───────────────────────────────────────────────────────────────

/// <summary>Base UI widget wrapper.</summary>
[NativeProperty("IsActive", "bool", "WidgetControl_GetActive", "WidgetControl_SetActive")]
[NativeProperty("TextColorRGBA", "string", "WidgetControl_GetTextColor")]
[NativeCall("Chained.WidgetControl", "WidgetControl_SetTextColorRGBA", "void", "ulong", "int", "int", "int", "int")]
public partial class WidgetControl : Component
{
    public void SetTextColor(int r, int g, int b, int a = 255)
    {
        unsafe { if (WidgetControl_SetTextColorRGBA_Ptr != null) WidgetControl_SetTextColorRGBA_Ptr(Entity.ID, r, g, b, a); }
    }
}

/// <summary>Button control wrapper.</summary>
[NativeProperty("IsClicked", "bool", "ButtonControl_IsClicked")]
[NativeProperty("IsDown", "bool", "ButtonControl_IsDown")]
[NativeProperty("Label", "string", "ButtonControl_GetLabel", "ButtonControl_SetLabel")]
public partial class ButtonControl : WidgetControl
{
}

/// <summary>Label control wrapper.</summary>
[NativeProperty("Text", "string", "LabelControl_GetText", "LabelControl_SetText")]
public partial class LabelControl : WidgetControl
{
}

/// <summary>Checkbox wrapper.</summary>
[NativeProperty("IsChecked", "bool", "CheckboxControl_GetChecked", "CheckboxControl_SetChecked")]
public partial class CheckboxControl : WidgetControl
{
}

/// <summary>Slider control wrapper.</summary>
[NativeProperty("Label", "string", "SliderControl_GetLabel", "SliderControl_SetLabel")]
[NativeProperty("Value", "float", "SliderControl_GetValue", "SliderControl_SetValue")]
[NativeProperty("Min", "float", "SliderControl_GetMin", "SliderControl_SetMin")]
[NativeProperty("Max", "float", "SliderControl_GetMax", "SliderControl_SetMax")]
public partial class SliderControl : WidgetControl
{
}

/// <summary>Progress bar wrapper.</summary>
[NativeProperty("Progress", "float", "ProgressBarControl_GetProgress", "ProgressBarControl_SetProgress")]
[NativeProperty("OverlayText", "string", "ProgressBarControl_GetOverlayText", "ProgressBarControl_SetOverlayText")]
[NativeProperty("ShowPercentage", "bool", "ProgressBarControl_GetShowPercentage", "ProgressBarControl_SetShowPercentage")]
public partial class ProgressBarControl : WidgetControl
{
}

/// <summary>Combo box wrapper.</summary>
[NativeProperty("SelectedIndex", "int", "ComboBoxControl_GetSelectedIndex", "ComboBoxControl_SetSelectedIndex")]
[NativeProperty("ItemCount", "int", "ComboBoxControl_GetItemCount")]
[NativeCall("Chained.ComboBoxControl", "ComboBoxControl_AddItem", "void", "ulong", "char*")]
[NativeCall("Chained.ComboBoxControl", "ComboBoxControl_ClearItems", "void", "ulong")]
[NativeCall("Chained.ComboBoxControl", "ComboBoxControl_GetItem", "char*", "ulong", "int")]
public partial class ComboBoxControl : WidgetControl
{
    public void AddItem(string item)
    {
        unsafe { if (ComboBoxControl_AddItem_Ptr != null) fixed (char* ptr = item) ComboBoxControl_AddItem_Ptr(Entity.ID, ptr); }
    }

    public void ClearItems()
    {
        unsafe { if (ComboBoxControl_ClearItems_Ptr != null) ComboBoxControl_ClearItems_Ptr(Entity.ID); }
    }

    public string? GetItem(int index)
    {
        unsafe
        {
            if (ComboBoxControl_GetItem_Ptr == null) return null;
            return Marshal.PtrToStringUni(new IntPtr(ComboBoxControl_GetItem_Ptr(Entity.ID, index))) ?? string.Empty;
        }
    }
}

/// <summary>Input text field wrapper.</summary>
[NativeProperty("Text", "string", "InputTextControl_GetText", "InputTextControl_SetText")]
[NativeProperty("HasChanged", "bool", "InputTextControl_HasChanged")]
[NativeCall("Chained.InputTextControl", "InputTextControl_ClearChanged", "void", "ulong")]
public partial class InputTextControl : WidgetControl
{
    public void ClearChanged()
    {
        unsafe { if (InputTextControl_ClearChanged_Ptr != null) InputTextControl_ClearChanged_Ptr(Entity.ID); }
    }
}

}
