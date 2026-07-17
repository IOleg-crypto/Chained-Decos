using System;
using System.Runtime.InteropServices;
using Coral.Managed.Interop;

namespace Chained
{

// ── UI Controls ───────────────────────────────────────────────────────────────

/// <summary>Base UI widget wrapper.</summary>
public class WidgetControl : Component
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, byte> WidgetControl_GetActive_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> WidgetControl_SetActive_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*> WidgetControl_GetTextColor_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, int, int, int, void> WidgetControl_SetTextColorRGBA_Ptr;
#pragma warning restore 0649

    public bool IsActive
    {
        get { unsafe { return WidgetControl_GetActive_Ptr(Entity.ID) != 0; } }
        set { unsafe { WidgetControl_SetActive_Ptr(Entity.ID, value); } }
    }

    public void SetTextColor(int r, int g, int b, int a = 255)
    {
        unsafe { WidgetControl_SetTextColorRGBA_Ptr(Entity.ID, r, g, b, a); }
    }

    public string TextColorRGBA
    {
        get { unsafe { return Marshal.PtrToStringUni(new IntPtr(WidgetControl_GetTextColor_Ptr(Entity.ID))) ?? string.Empty; } }
    }
}

/// <summary>Button control wrapper.</summary>
public class ButtonControl : WidgetControl
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, byte> ButtonControl_IsClicked_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> ButtonControl_IsDown_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*> ButtonControl_GetLabel_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> ButtonControl_SetLabel_Ptr;
#pragma warning restore 0649

    public bool IsClicked { get { unsafe { return ButtonControl_IsClicked_Ptr(Entity.ID) != 0; } } }
    public bool IsDown    { get { unsafe { return ButtonControl_IsDown_Ptr(Entity.ID) != 0; } } }

    public string Label
    {
        get { unsafe { return Marshal.PtrToStringUni(new IntPtr(ButtonControl_GetLabel_Ptr(Entity.ID))) ?? string.Empty; } }
        set { unsafe { fixed (char* ptr = value) ButtonControl_SetLabel_Ptr(Entity.ID, ptr); } }
    }
}

/// <summary>Label control wrapper.</summary>
public class LabelControl : WidgetControl
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*> LabelControl_GetText_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> LabelControl_SetText_Ptr;
#pragma warning restore 0649

    public string Text
    {
        get { unsafe { return Marshal.PtrToStringUni(new IntPtr(LabelControl_GetText_Ptr(Entity.ID))) ?? string.Empty; } }
        set { unsafe { fixed (char* ptr = value) LabelControl_SetText_Ptr(Entity.ID, ptr); } }
    }
}

/// <summary>Checkbox wrapper.</summary>
public class CheckboxControl : WidgetControl
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, byte> CheckboxControl_GetChecked_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> CheckboxControl_SetChecked_Ptr;
#pragma warning restore 0649

    public bool IsChecked
    {
        get { unsafe { return CheckboxControl_GetChecked_Ptr(Entity.ID) != 0; } }
        set { unsafe { CheckboxControl_SetChecked_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Slider control wrapper.</summary>
public class SliderControl : WidgetControl
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, char*> SliderControl_GetLabel_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> SliderControl_SetLabel_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> SliderControl_GetValue_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> SliderControl_SetValue_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> SliderControl_GetMin_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> SliderControl_SetMin_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float> SliderControl_GetMax_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> SliderControl_SetMax_Ptr;
#pragma warning restore 0649

    public string Label
    {
        get { unsafe { return Marshal.PtrToStringUni(new IntPtr(SliderControl_GetLabel_Ptr(Entity.ID))) ?? string.Empty; } }
        set { unsafe { fixed (char* ptr = value) SliderControl_SetLabel_Ptr(Entity.ID, ptr); } }
    }
    public float Value
    {
        get { unsafe { return SliderControl_GetValue_Ptr(Entity.ID); } }
        set { unsafe { SliderControl_SetValue_Ptr(Entity.ID, value); } }
    }
    public float Min
    {
        get { unsafe { return SliderControl_GetMin_Ptr(Entity.ID); } }
        set { unsafe { SliderControl_SetMin_Ptr(Entity.ID, value); } }
    }
    public float Max
    {
        get { unsafe { return SliderControl_GetMax_Ptr(Entity.ID); } }
        set { unsafe { SliderControl_SetMax_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Progress bar wrapper.</summary>
public class ProgressBarControl : WidgetControl
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, float> ProgressBarControl_GetProgress_Ptr;
    internal static unsafe delegate* unmanaged<ulong, float, void> ProgressBarControl_SetProgress_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*> ProgressBarControl_GetOverlayText_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> ProgressBarControl_SetOverlayText_Ptr;
    internal static unsafe delegate* unmanaged<ulong, byte> ProgressBarControl_GetShowPercentage_Ptr;
    internal static unsafe delegate* unmanaged<ulong, bool, void> ProgressBarControl_SetShowPercentage_Ptr;
#pragma warning restore 0649

    public float Progress
    {
        get { unsafe { return ProgressBarControl_GetProgress_Ptr(Entity.ID); } }
        set { unsafe { ProgressBarControl_SetProgress_Ptr(Entity.ID, value); } }
    }
    public string OverlayText
    {
        get { unsafe { return Marshal.PtrToStringUni(new IntPtr(ProgressBarControl_GetOverlayText_Ptr(Entity.ID))) ?? string.Empty; } }
        set { unsafe { fixed (char* ptr = value) ProgressBarControl_SetOverlayText_Ptr(Entity.ID, ptr); } }
    }
    public bool ShowPercentage
    {
        get { unsafe { return ProgressBarControl_GetShowPercentage_Ptr(Entity.ID) != 0; } }
        set { unsafe { ProgressBarControl_SetShowPercentage_Ptr(Entity.ID, value); } }
    }
}

/// <summary>Combo box wrapper.</summary>
public class ComboBoxControl : WidgetControl
{
#pragma warning disable 0649
    internal static unsafe delegate* unmanaged<ulong, int> ComboBoxControl_GetSelectedIndex_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, void> ComboBoxControl_SetSelectedIndex_Ptr;
    internal static unsafe delegate* unmanaged<ulong, char*, void> ComboBoxControl_AddItem_Ptr;
    internal static unsafe delegate* unmanaged<ulong, void> ComboBoxControl_ClearItems_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int> ComboBoxControl_GetItemCount_Ptr;
    internal static unsafe delegate* unmanaged<ulong, int, char*> ComboBoxControl_GetItem_Ptr;
#pragma warning restore 0649

    private static unsafe int GetSelectedIndex(ulong entityID) => ComboBoxControl_GetSelectedIndex_Ptr(entityID);
    private static unsafe void SetSelectedIndex(ulong entityID, int index) => ComboBoxControl_SetSelectedIndex_Ptr(entityID, index);
    private static unsafe void AddItem(ulong entityID, string item)
    { if (ComboBoxControl_AddItem_Ptr != null) fixed (char* ptr = item) ComboBoxControl_AddItem_Ptr(entityID, ptr); }
    private static unsafe void ClearItems(ulong entityID) => ComboBoxControl_ClearItems_Ptr(entityID);
    private static unsafe int GetItemCount(ulong entityID) => ComboBoxControl_GetItemCount_Ptr(entityID);
    private static unsafe string GetItem(ulong entityID, int index) => Marshal.PtrToStringUni(new IntPtr(ComboBoxControl_GetItem_Ptr(entityID, index))) ?? string.Empty;

    public int    SelectedIndex     { get => GetSelectedIndex(Entity.ID); set => SetSelectedIndex(Entity.ID, value); }
    public int    ItemCount         => GetItemCount(Entity.ID);
    public string? GetItem(int index) => GetItem(Entity.ID, index);
    public void   AddItem(string item) => AddItem(Entity.ID, item);
    public void   ClearItems()         => ClearItems(Entity.ID);
}

}
