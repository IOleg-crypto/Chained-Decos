#include "component_serializer.h"
#include "scene.h"
#include "components.h"
#include "engine/core/log.h"
#include "engine/core/yaml.h"
#include "serialization_utils.h"
#include <fstream>

namespace CHEngine
{
    using namespace SerializationUtils;

    // ========================================================================
    // Допоміжні функції для TextStyle та UIStyle (залишаємо для Nested)
    // ========================================================================
    
    static void serializeTextStyle(YAML::Emitter& out, const TextStyle& style) {
        out << YAML::BeginMap;
        out << YAML::Key << "FontName" << YAML::Value << style.FontName;
        out << YAML::Key << "FontSize" << YAML::Value << style.FontSize;
        out << YAML::Key << "TextColor" << YAML::Value << style.TextColor;
        out << YAML::Key << "Shadow" << YAML::Value << style.Shadow;
        out << YAML::Key << "ShadowOffset" << YAML::Value << style.ShadowOffset;
        out << YAML::Key << "ShadowColor" << YAML::Value << style.ShadowColor;
        out << YAML::Key << "LetterSpacing" << YAML::Value << style.LetterSpacing;
        out << YAML::Key << "LineHeight" << YAML::Value << style.LineHeight;
        out << YAML::Key << "HorizontalAlignment" << YAML::Value << (int)style.HorizontalAlignment;
        out << YAML::Key << "VerticalAlignment" << YAML::Value << (int)style.VerticalAlignment;
        out << YAML::EndMap;
    }

    static void deserializeTextStyle(TextStyle& style, YAML::Node node) {
        if (node["FontName"]) style.FontName = node["FontName"].template as<std::string>();
        if (node["FontSize"]) style.FontSize = node["FontSize"].template as<float>();
        if (node["TextColor"]) style.TextColor = node["TextColor"].template as<Color>();
        if (node["Shadow"]) style.Shadow = node["Shadow"].template as<bool>();
        if (node["ShadowOffset"]) style.ShadowOffset = node["ShadowOffset"].template as<float>();
        if (node["ShadowColor"]) style.ShadowColor = node["ShadowColor"].template as<Color>();
        if (node["LetterSpacing"]) style.LetterSpacing = node["LetterSpacing"].template as<float>();
        if (node["LineHeight"]) style.LineHeight = node["LineHeight"].template as<float>();
        
        if (node["HorizontalAlignment"]) style.HorizontalAlignment = (TextAlignment)node["HorizontalAlignment"].template as<int>();
        else if (node["HAlign"]) style.HorizontalAlignment = (TextAlignment)node["HAlign"].template as<int>();

        if (node["VerticalAlignment"]) style.VerticalAlignment = (TextAlignment)node["VerticalAlignment"].template as<int>();
        else if (node["VAlign"]) style.VerticalAlignment = (TextAlignment)node["VAlign"].template as<int>();
    }

    static void serializeUIStyle(YAML::Emitter& out, const UIStyle& style) {
        out << YAML::BeginMap;
        out << YAML::Key << "BackgroundColor" << YAML::Value << style.BackgroundColor;
        out << YAML::Key << "HoverColor" << YAML::Value << style.HoverColor;
        out << YAML::Key << "PressedColor" << YAML::Value << style.PressedColor;
        out << YAML::Key << "Rounding" << YAML::Value << style.Rounding;
        out << YAML::Key << "BorderSize" << YAML::Value << style.BorderSize;
        out << YAML::Key << "BorderColor" << YAML::Value << style.BorderColor;
        out << YAML::Key << "Padding" << YAML::Value << style.Padding;
        out << YAML::EndMap;
    }

    static void deserializeUIStyle(UIStyle& style, YAML::Node node) {
        if (node["BackgroundColor"]) style.BackgroundColor = node["BackgroundColor"].template as<Color>();
        else if (node["BGColor"]) style.BackgroundColor = node["BGColor"].template as<Color>();

        if (node["HoverColor"]) style.HoverColor = node["HoverColor"].template as<Color>();
        if (node["PressedColor"]) style.PressedColor = node["PressedColor"].template as<Color>();
        if (node["Rounding"]) style.Rounding = node["Rounding"].template as<float>();
        if (node["BorderSize"]) style.BorderSize = node["BorderSize"].template as<float>();
        if (node["BorderColor"]) style.BorderColor = node["BorderColor"].template as<Color>();
        if (node["Padding"]) style.Padding = node["Padding"].template as<float>();
    }

    // ========================================================================
    // Декларативні описи компонентів (Layout Functions)
    // ========================================================================

    static void LayoutButtonControl(PropertyArchive& archive, ButtonControl& component)
    {
        archive("Label", component.Label);
        archive("Interactable", component.IsInteractable);
        archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
        archive.Nested("Text", component.Text, serializeTextStyle, deserializeTextStyle);
    }

    static void LayoutPanelControl(PropertyArchive& archive, PanelControl& component)
    {
        archive.Path("TexturePath", component.TexturePath);
        archive.Nested("UIStyle", component.Style, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutLabelControl(PropertyArchive& archive, LabelControl& component)
    {
        archive("Text", component.Text);
        archive.Nested("Style", component.Style, serializeTextStyle, deserializeTextStyle);
    }

    static void LayoutSliderControl(PropertyArchive& archive, SliderControl& component)
    {
        archive("Label", component.Label)
               ("Value", component.Value)
               ("Min", component.Min)
               ("Max", component.Max);
        archive.Nested("Text", component.Text, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutCheckboxControl(PropertyArchive& archive, CheckboxControl& component)
    {
        archive("Label", component.Label)
               ("Checked", component.Checked);
        archive.Nested("Text", component.Text, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutInputTextControl(PropertyArchive& archive, InputTextControl& component)
    {
        archive("Label", component.Label)
               ("Text", component.Text)
               ("Placeholder", component.Placeholder)
               ("MaxLength", component.MaxLength)
               ("Multiline", component.Multiline)
               ("ReadOnly", component.ReadOnly)
               ("Password", component.Password);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutComboBoxControl(PropertyArchive& archive, ComboBoxControl& component)
    {
        archive("Label", component.Label)
               ("SelectedIndex", component.SelectedIndex);
        archive.Sequence("Items", component.Items);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutProgressBarControl(PropertyArchive& archive, ProgressBarControl& component)
    {
        archive("Progress", component.Progress)
               ("OverlayText", component.OverlayText)
               ("ShowPercentage", component.ShowPercentage);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.BarStyle, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutImageControl(PropertyArchive& archive, ImageControl& component)
    {
        archive.Path("TexturePath", component.TexturePath);
        archive("Size", component.Size)
               ("TintColor", component.TintColor)
               ("BorderColor", component.BorderColor);
        archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutImageButtonControl(PropertyArchive& archive, ImageButtonControl& component)
    {
        archive("Label", component.Label);
        archive.Path("TexturePath", component.TexturePath);
        archive("Size", component.Size)
               ("TintColor", component.TintColor)
               ("BackgroundColor", component.BackgroundColor)
               ("FramePadding", component.FramePadding);
        archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutSeparatorControl(PropertyArchive& archive, SeparatorControl& component)
    {
        archive("Thickness", component.Thickness)
               ("LineColor", component.LineColor);
    }

    static void LayoutRadioButtonControl(PropertyArchive& archive, RadioButtonControl& component)
    {
        archive("Label", component.Label)
               ("SelectedIndex", component.SelectedIndex)
               ("Horizontal", component.Horizontal);
        archive.Sequence("Options", component.Options);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
    }

    static void LayoutColorPickerControl(PropertyArchive& archive, ColorPickerControl& component)
    {
        archive("Label", component.Label)
               ("SelectedColor", component.SelectedColor)
               ("ShowAlpha", component.ShowAlpha)
               ("ShowPicker", component.ShowPicker);
        archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutDragFloatControl(PropertyArchive& archive, DragFloatControl& component)
    {
        archive("Label", component.Label)
               ("Value", component.Value)
               ("Speed", component.Speed)
               ("Min", component.Min)
               ("Max", component.Max)
               ("Format", component.Format);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutDragIntControl(PropertyArchive& archive, DragIntControl& component)
    {
        archive("Label", component.Label)
               ("Value", component.Value)
               ("Speed", component.Speed)
               ("Min", component.Min)
               ("Max", component.Max)
               ("Format", component.Format);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutTreeNodeControl(PropertyArchive& archive, TreeNodeControl& component)
    {
        archive("Label", component.Label)
               ("DefaultOpen", component.DefaultOpen)
               ("IsLeaf", component.IsLeaf);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
    }

    static void LayoutTabBarControl(PropertyArchive& archive, TabBarControl& component)
    {
        archive("Label", component.Label)
               ("Reorderable", component.Reorderable)
               ("AutoSelectNewTabs", component.AutoSelectNewTabs);
        archive.Nested("Style", component.Style, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutTabItemControl(PropertyArchive& archive, TabItemControl& component)
    {
        archive("Label", component.Label)
               ("IsOpen", component.IsOpen);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
    }

    static void LayoutCollapsingHeaderControl(PropertyArchive& archive, CollapsingHeaderControl& component)
    {
        archive("Label", component.Label)
               ("DefaultOpen", component.DefaultOpen);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
    }

    static void LayoutPlotLinesControl(PropertyArchive& archive, PlotLinesControl& component)
    {
        archive("Label", component.Label)
               ("OverlayText", component.OverlayText)
               ("ScaleMin", component.ScaleMin)
               ("ScaleMax", component.ScaleMax)
               ("GraphSize", component.GraphSize);
        archive.Sequence("Values", component.Values);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
    }

    static void LayoutPlotHistogramControl(PropertyArchive& archive, PlotHistogramControl& component)
    {
        archive("Label", component.Label)
               ("OverlayText", component.OverlayText)
               ("ScaleMin", component.ScaleMin)
               ("ScaleMax", component.ScaleMax)
               ("GraphSize", component.GraphSize);
        archive.Sequence("Values", component.Values);
        archive.Nested("Text", component.Style, serializeTextStyle, deserializeTextStyle);
        archive.Nested("Style", component.BoxStyle, serializeUIStyle, deserializeUIStyle);
    }

    // ========================================================================
    // Registration of UI components (using PropertyArchive)
    // ========================================================================

    void ComponentSerializer::RegisterUIComponents()
    {
        Register<ButtonControl>("ButtonControl", LayoutButtonControl);
        Register<PanelControl>("PanelControl", LayoutPanelControl);
        Register<LabelControl>("LabelControl", LayoutLabelControl);
        Register<SliderControl>("SliderControl", LayoutSliderControl);
        Register<CheckboxControl>("CheckboxControl", LayoutCheckboxControl);
        Register<InputTextControl>("InputTextControl", LayoutInputTextControl);
        Register<ComboBoxControl>("ComboBoxControl", LayoutComboBoxControl);
        Register<ProgressBarControl>("ProgressBarControl", LayoutProgressBarControl);
        Register<ImageControl>("ImageControl", LayoutImageControl);
        Register<ImageButtonControl>("ImageButtonControl", LayoutImageButtonControl);
        Register<SeparatorControl>("SeparatorControl", LayoutSeparatorControl);
        Register<RadioButtonControl>("RadioButtonControl", LayoutRadioButtonControl);
        Register<ColorPickerControl>("ColorPickerControl", LayoutColorPickerControl);
        Register<DragFloatControl>("DragFloatControl", LayoutDragFloatControl);
        Register<DragIntControl>("DragIntControl", LayoutDragIntControl);
        Register<TreeNodeControl>("TreeNodeControl", LayoutTreeNodeControl);
        Register<TabBarControl>("TabBarControl", LayoutTabBarControl);
        Register<TabItemControl>("TabItemControl", LayoutTabItemControl);
        Register<CollapsingHeaderControl>("CollapsingHeaderControl", LayoutCollapsingHeaderControl);
        Register<PlotLinesControl>("PlotLinesControl", LayoutPlotLinesControl);
        Register<PlotHistogramControl>("PlotHistogramControl", LayoutPlotHistogramControl);
    }
}
