#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/graphics/pipeline/ui_style.h"
#include <variant>

namespace CHEngine
{


struct TextStyle
{
    std::string FontName = "Default";
    float FontSize = 18.0f;
    Color TextColor = { 255, 255, 255, 255 };
    bool Shadow = false;
    float ShadowOffset = 2.0f;
    Color ShadowColor = { 0, 0, 0, 255 };
    float LetterSpacing = { 1.0f };
    float LineHeight = 1.2f;
    HorizontalAlignment Horizontal = HorizontalAlignment::Center;
    VerticalAlignment Vertical = VerticalAlignment::Center;

    CH_REFLECT_BEGIN(TextStyle)
        CH_PROP_NAMED(props, "Font Name", FontName);
        CH_PROP_META_NAMED(props, "Font Size", FontSize, PropertyMeta(6.0f, 128.0f, 1.0f));
        CH_PROP_NAMED(props, "Text Color", TextColor);
        CH_PROP_NAMED(props, "Shadow", Shadow);
        if (Shadow)
        {
            CH_PROP_META_NAMED(props, "Shadow Offset", ShadowOffset, PropertyMeta(0.0f, 20.0f, 0.1f));
            CH_PROP_NAMED(props, "Shadow Color", ShadowColor);
        }
        CH_PROP_META_NAMED(props, "Letter Spacing", LetterSpacing, PropertyMeta(0.5f, 4.0f, 0.1f));
        CH_PROP_META_NAMED(props, "Line Height", LineHeight, PropertyMeta(0.5f, 3.0f, 0.1f));
        // Alignments as ints for now
        CH_PROP_NAMED(props, "H Align", (int&)Horizontal);
        CH_PROP_NAMED(props, "V Align", (int&)Vertical);
    CH_REFLECT_END()
};

struct UIStyle
{
    Color BackgroundColor = {40, 40, 40, 255};
    Color HoverColor = {60, 60, 60, 255};
    Color PressedColor = {30, 30, 30, 255};

    float Rounding = 4.0f;
    float BorderSize = 0.0f;
    Color BorderColor = { 255, 255, 255, 255 };

    bool UseGradient = false;
    Color GradientColor = {20, 20, 20, 255};

    float Padding = 4.0f;

    float HoverScale = 1.0f;
    float PressedScale = 1.0f;
    float TransitionSpeed = 0.1f;

    CH_REFLECT_BEGIN(UIStyle)
        CH_PROP_NAMED(props, "BG Color", BackgroundColor);
        CH_PROP_NAMED(props, "Hover Color", HoverColor);
        CH_PROP_NAMED(props, "Pressed Color", PressedColor);
        CH_PROP_META_NAMED(props, "Rounding", Rounding, PropertyMeta(0.0f, 20.0f, 0.5f));
        CH_PROP_META_NAMED(props, "Border Size", BorderSize, PropertyMeta(0.0f, 10.0f, 0.1f));
        CH_PROP_NAMED(props, "Border Color", BorderColor);
        CH_PROP_NAMED(props, "Gradient", UseGradient);
        if (UseGradient)
            CH_PROP_NAMED(props, "Gradient Color", GradientColor);
        CH_PROP_META_NAMED(props, "Padding", Padding, PropertyMeta(0.0f, 50.0f, 1.0f));
        CH_PROP_META_NAMED(props, "Hover Scale", HoverScale, PropertyMeta(0.8f, 1.5f, 0.05f));
        CH_PROP_META_NAMED(props, "Pressed Scale", PressedScale, PropertyMeta(0.8f, 1.5f, 0.05f));
        CH_PROP_META_NAMED(props, "Transition Speed", TransitionSpeed, PropertyMeta(0.01f, 1.0f, 0.01f));
    CH_REFLECT_END()

    // Runtime state (not serialized)
    struct RuntimeState {
        float AnimationAlpha = 0.0f; // 0 = idle, 1 = hover/pressed
        float CurrentScale = 1.0f;
        Color CurrentColor = {255, 255, 255, 255};
    } State;
};


struct RectTransform
{
    glm::vec2 AnchorMin = {0.5f, 0.5f};
    glm::vec2 AnchorMax = {0.5f, 0.5f};
    glm::vec2 OffsetMin = {-50.0f, -20.0f};
    glm::vec2 OffsetMax = {50.0f, 20.0f};
    glm::vec2 Pivot = {0.5f, 0.5f};
    float Rotation = 0.0f;
    glm::vec2 Scale = {1.0f, 1.0f};

    CH_REFLECT_BEGIN(RectTransform)
        bool changed = false;
        bool isFill = (AnchorMin.x == 0.0f && AnchorMax.x == 1.0f && AnchorMin.y == 0.0f && AnchorMax.y == 1.0f);
        
        if (CH_BEGIN_GROUP(props, "Simplified Layout", !isFill))
        {
            if (isFill)
            {
                CH_HEADER(props, "Fill Mode Active");
                CH_PROP_META_NAMED(props, "Padding L/R", OffsetMin.x, PropertyMeta(-1000.0f, 1000.0f, 1.0f)); // Simplified view
                CH_PROP_META_NAMED(props, "Padding T/B", OffsetMin.y, PropertyMeta(-1000.0f, 1000.0f, 1.0f));
            }
            else
            {
                glm::vec2 pos = (OffsetMin + OffsetMax) * 0.5f;
                glm::vec2 size = OffsetMax - OffsetMin;
                
                if (CH_PROP_META_NAMED(props, "Position", pos, PropertyMeta(-2000.0f, 2000.0f, 1.0f)))
                {
                    OffsetMin.x = pos.x - size.x * Pivot.x;
                    OffsetMin.y = pos.y - size.y * Pivot.y;
                    OffsetMax.x = pos.x + size.x * (1.0f - Pivot.x);
                    OffsetMax.y = pos.y + size.y * (1.0f - Pivot.y);
                    changed = true;
                }
                
                if (CH_PROP_META_NAMED(props, "Size", size, PropertyMeta(1.0f, 2000.0f, 1.0f)))
                {
                    OffsetMin.x = pos.x - size.x * Pivot.x;
                    OffsetMin.y = pos.y - size.y * Pivot.y;
                    OffsetMax.x = pos.x + size.x * (1.0f - Pivot.x);
                    OffsetMax.y = pos.y + size.y * (1.0f - Pivot.y);
                    changed = true;
                }
            }
            CH_END_GROUP(props);
        }

        if (CH_BEGIN_GROUP(props, "Advanced Settings", isFill))
        {
            if (CH_PROP_NAMED(props, "Anchor Min", AnchorMin)) changed = true;
            if (CH_PROP_NAMED(props, "Anchor Max", AnchorMax)) changed = true;
            if (CH_PROP_META_NAMED(props, "Offset Min", OffsetMin, PropertyMeta(-2000.0f, 2000.0f, 1.0f))) changed = true;
            if (CH_PROP_META_NAMED(props, "Offset Max", OffsetMax, PropertyMeta(-2000.0f, 2000.0f, 1.0f))) changed = true;
            if (CH_PROP_NAMED(props, "Pivot", Pivot)) changed = true;
            if (CH_PROP_META_NAMED(props, "Rotation", Rotation, PropertyMeta(-360.0f, 360.0f, 1.0f))) changed = true;
            if (CH_PROP_META_NAMED(props, "Scale", Scale, PropertyMeta(0.1f, 10.0f, 0.1f))) changed = true;
            CH_END_GROUP(props);
        }
        
        if (changed) props.SetChanged(true);
    CH_REFLECT_END()
};

struct ControlComponent
{
    RectTransform Transform;
    int32_t ZOrder = 0;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    ControlComponent() = default;

    CH_REFLECT_BEGIN(ControlComponent)
        CH_NESTED_NAMED(props, "Rect Transform", Transform);
        CH_PROP_META_NAMED(props, "Z Order", ZOrder, PropertyMeta(-1000.0f, 1000.0f, 1.0f));
        CH_PROP_NAMED(props, "Active", IsActive);
        CH_PROP_NAMED(props, "Hidden", HiddenInHierarchy);
    CH_REFLECT_END()
};

struct ButtonData
{
    std::string Label = "Button";
    bool IsInteractable = true;
    bool AutoSize = false;
};

struct PanelData
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    bool FullScreen = false;
};

struct LabelData
{
    std::string Text = "Text Label";
    bool AutoSize = false;
};

struct SliderData
{
    std::string Label = "Slider";
    float Value = 0.5f;
    float Min = 0.0f;
    float Max = 1.0f;
};

struct CheckboxData
{
    std::string Label = "Checkbox";
    bool Checked = false;
};

struct InputTextData
{
    std::string Label = "Input";
    std::string Text = "";
    std::string Placeholder = "Enter text...";
    int MaxLength = 256;
    bool Multiline = false;
    bool ReadOnly = false;
    bool Password = false;
    std::vector<char> InputBuffer;
};

struct ComboBoxData
{
    std::string Label = "Combo";
    std::vector<std::string> Items = {"Option 1", "Option 2", "Option 3"};
    int SelectedIndex = 0;
};

struct ProgressBarData
{
    float Progress = 0.5f;
    std::string OverlayText = "";
    bool ShowPercentage = true;
};

struct ImageData
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    Color TintColor = {255, 255, 255, 255};
    Color BorderColor = {0, 0, 0, 0};
};

struct ImageButtonData
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    std::string Label = "ImageButton";
    Color TintColor = {255, 255, 255, 255};
    Color BackgroundColor = {0, 0, 0, 0};
    int FramePadding = -1;
};

struct SeparatorData
{
    float Thickness = 1.0f;
    Color LineColor = {127, 127, 127, 255};
};

struct RadioButtonData
{
    std::string Label = "RadioGroup";
    std::vector<std::string> Options = {"Option 1", "Option 2", "Option 3"};
    int SelectedIndex = 0;
    bool Horizontal = false;
};

struct ColorPickerData
{
    std::string Label = "Color";
    Color SelectedColor = {255, 255, 255, 255};
    bool ShowAlpha = true;
    bool ShowPicker = true;
};

struct DragFloatData
{
    std::string Label = "DragFloat";
    float Value = 0.0f;
    float Speed = 0.1f;
    float Min = 0.0f;
    float Max = 100.0f;
    std::string Format = "%.3f";
};

struct DragIntData
{
    std::string Label = "DragInt";
    int Value = 0;
    float Speed = 1.0f;
    int Min = 0;
    int Max = 100;
    std::string Format = "%d";
};

struct TreeNodeData
{
    std::string Label = "TreeNode";
    bool IsOpen = false;
    bool DefaultOpen = false;
    bool IsLeaf = false;
};

struct TabBarData
{
    std::string Label = "TabBar";
    bool Reorderable = true;
    bool AutoSelectNewTabs = true;
};

struct TabItemData
{
    std::string Label = "Tab";
    bool IsOpen = true;
    bool Selected = false;
};

struct CollapsingHeaderData
{
    std::string Label = "Header";
    bool IsOpen = false;
    bool DefaultOpen = false;
};

struct PlotLinesData
{
    std::string Label = "Plot";
    std::vector<float> Values = {0.0f, 0.5f, 1.0f, 0.5f, 0.0f};
    std::string OverlayText = "";
    float ScaleMin = 0.0f;
    float ScaleMax = 1.0f;
    glm::vec2 GraphSize = {0, 80};
};

struct PlotHistogramData
{
    std::string Label = "Histogram";
    std::vector<float> Values = {0.2f, 0.5f, 0.8f, 0.4f, 0.6f};
    std::string OverlayText = "";
    float ScaleMin = 0.0f;
    float ScaleMax = 1.0f;
    glm::vec2 GraphSize = {0, 80};
};

struct VerticalLayoutGroupData
{
    float Spacing = 10.0f;
    glm::vec2 Padding = {10, 10};
};

using WidgetData = std::variant<
    std::monostate,
    ButtonData,
    PanelData,
    LabelData,
    SliderData,
    CheckboxData,
    InputTextData,
    ComboBoxData,
    ProgressBarData,
    ImageData,
    ImageButtonData,
    SeparatorData,
    RadioButtonData,
    ColorPickerData,
    DragFloatData,
    DragIntData,
    TreeNodeData,
    TabBarData,
    TabItemData,
    CollapsingHeaderData,
    PlotLinesData,
    PlotHistogramData,
    VerticalLayoutGroupData
>;

struct WidgetComponent
{
    UIStyle BoxStyle;
    TextStyle TextStyle;
    WidgetData Data = std::monostate{};

    bool IsHovered = false;
    bool IsDown = false;
    bool PressedThisFrame = false;
    bool ValueChanged = false;

    WidgetComponent() = default;


    CH_REFLECT_BEGIN(WidgetComponent)
        CH_NESTED_NAMED(props, "Box Style", BoxStyle);
        CH_NESTED_NAMED(props, "Text Style", TextStyle);

        int typeIndex = (int)Data.index();
        const char* typeNames[] = {
            "None", "Button", "Panel", "Label", "Slider", "Checkbox", "InputText",
            "ComboBox", "ProgressBar", "Image", "ImageButton", "Separator",
            "RadioButton", "ColorPicker", "DragFloat", "DragInt", "TreeNode",
            "TabBar", "TabItem", "CollapsingHeader", "PlotLines", "PlotHistogram",
            "VerticalLayoutGroup"
        };

        if (CH_ENUM_NAMED(props, "Widget Type", typeIndex, typeNames) ||
            props.GetMode() == CHEngine::ReflectionMode::Deserialize)
        {
            switch (typeIndex)
            {
                case 0: Data = std::monostate{}; break;
                case 1: Data = ButtonData{}; break;
                case 2: Data = PanelData{}; break;
                case 3: Data = LabelData{}; break;
                case 4: Data = SliderData{}; break;
                case 5: Data = CheckboxData{}; break;
                case 6: Data = InputTextData{}; break;
                case 7: Data = ComboBoxData{}; break;
                case 8: Data = ProgressBarData{}; break;
                case 9: Data = ImageData{}; break;
                case 10: Data = ImageButtonData{}; break;
                case 11: Data = SeparatorData{}; break;
                case 12: Data = RadioButtonData{}; break;
                case 13: Data = ColorPickerData{}; break;
                case 14: Data = DragFloatData{}; break;
                case 15: Data = DragIntData{}; break;
                case 16: Data = TreeNodeData{}; break;
                case 17: Data = TabBarData{}; break;
                case 18: Data = TabItemData{}; break;
                case 19: Data = CollapsingHeaderData{}; break;
                case 20: Data = PlotLinesData{}; break;
                case 21: Data = PlotHistogramData{}; break;
                case 22: Data = VerticalLayoutGroupData{}; break;
            }
        }

        std::visit([&](auto& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ButtonData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Interactable", arg.IsInteractable);
                CH_PROP_NAMED(props, "Auto Size", arg.AutoSize);
            }
            else if constexpr (std::is_same_v<T, PanelData>) {
                if (props.GetMode() != CHEngine::ReflectionMode::UI)
                    CH_HANDLE_NAMED(props, "Texture Handle", arg.TextureHandle);
                CH_FILE_NAMED(props, "Texture Path", arg.TexturePath, "png,jpg,tga");
                CH_PROP_NAMED(props, "Full Screen", arg.FullScreen);
            }
            else if constexpr (std::is_same_v<T, LabelData>) {
                CH_PROP_NAMED(props, "Text", arg.Text);
                CH_PROP_NAMED(props, "Auto Size", arg.AutoSize);
            }
            else if constexpr (std::is_same_v<T, SliderData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_META_NAMED(props, "Value", arg.Value, PropertyMeta(0.0f, 100.0f, 0.1f));
                CH_PROP_META_NAMED(props, "Min", arg.Min, PropertyMeta(-100.0f, 100.0f, 0.1f));
                CH_PROP_META_NAMED(props, "Max", arg.Max, PropertyMeta(-100.0f, 100.0f, 0.1f));
            }
            else if constexpr (std::is_same_v<T, CheckboxData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Checked", arg.Checked);
            }
            else if constexpr (std::is_same_v<T, InputTextData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Text", arg.Text);
                CH_PROP_NAMED(props, "Placeholder", arg.Placeholder);
                CH_PROP_NAMED(props, "MaxLength", arg.MaxLength);
                CH_PROP_NAMED(props, "Multiline", arg.Multiline);
                CH_PROP_NAMED(props, "ReadOnly", arg.ReadOnly);
                CH_PROP_NAMED(props, "Password", arg.Password);
            }
            else if constexpr (std::is_same_v<T, ComboBoxData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_SEQUENCE_NAMED(props, "Items", arg.Items);
                CH_PROP_NAMED(props, "Selected Index", arg.SelectedIndex);
            }
            else if constexpr (std::is_same_v<T, ProgressBarData>) {
                CH_PROP_NAMED(props, "Progress", arg.Progress);
                CH_PROP_NAMED(props, "Overlay Text", arg.OverlayText);
                CH_PROP_NAMED(props, "Show Percentage", arg.ShowPercentage);
            }
            else if constexpr (std::is_same_v<T, ImageData>) {
                CH_FILE_NAMED(props, "Texture Path", arg.TexturePath, "png,jpg,tga");
                CH_PROP_NAMED(props, "Tint Color", arg.TintColor);
                CH_PROP_NAMED(props, "Border Color", arg.BorderColor);
            }
            else if constexpr (std::is_same_v<T, ImageButtonData>) {
                CH_FILE_NAMED(props, "Texture Path", arg.TexturePath, "png,jpg,tga");
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Tint Color", arg.TintColor);
                CH_PROP_NAMED(props, "Background Color", arg.BackgroundColor);
                CH_PROP_NAMED(props, "Frame Padding", arg.FramePadding);
            }
            else if constexpr (std::is_same_v<T, SeparatorData>) {
                CH_PROP_NAMED(props, "Thickness", arg.Thickness);
                CH_PROP_NAMED(props, "Line Color", arg.LineColor);
            }
            else if constexpr (std::is_same_v<T, RadioButtonData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_SEQUENCE_NAMED(props, "Options", arg.Options);
                CH_PROP_NAMED(props, "Selected Index", arg.SelectedIndex);
                CH_PROP_NAMED(props, "Horizontal", arg.Horizontal);
            }
            else if constexpr (std::is_same_v<T, ColorPickerData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Color", arg.SelectedColor);
                CH_PROP_NAMED(props, "Show Alpha", arg.ShowAlpha);
                CH_PROP_NAMED(props, "Show Picker", arg.ShowPicker);
            }
            else if constexpr (std::is_same_v<T, DragFloatData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Value", arg.Value);
                CH_PROP_NAMED(props, "Speed", arg.Speed);
                CH_PROP_NAMED(props, "Min", arg.Min);
                CH_PROP_NAMED(props, "Max", arg.Max);
            }
            else if constexpr (std::is_same_v<T, DragIntData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Value", arg.Value);
                CH_PROP_NAMED(props, "Speed", arg.Speed);
                CH_PROP_NAMED(props, "Min", arg.Min);
                CH_PROP_NAMED(props, "Max", arg.Max);
            }
            else if constexpr (std::is_same_v<T, TreeNodeData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Is Open", arg.IsOpen);
                CH_PROP_NAMED(props, "Default Open", arg.DefaultOpen);
                CH_PROP_NAMED(props, "Is Leaf", arg.IsLeaf);
            }
            else if constexpr (std::is_same_v<T, TabBarData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Reorderable", arg.Reorderable);
                CH_PROP_NAMED(props, "Auto Select New Tabs", arg.AutoSelectNewTabs);
            }
            else if constexpr (std::is_same_v<T, TabItemData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Is Open", arg.IsOpen);
                CH_PROP_NAMED(props, "Selected", arg.Selected);
            }
            else if constexpr (std::is_same_v<T, CollapsingHeaderData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_PROP_NAMED(props, "Is Open", arg.IsOpen);
                CH_PROP_NAMED(props, "Default Open", arg.DefaultOpen);
            }
            else if constexpr (std::is_same_v<T, PlotLinesData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_SEQUENCE_NAMED(props, "Values", arg.Values);
                CH_PROP_NAMED(props, "Overlay Text", arg.OverlayText);
                CH_PROP_NAMED(props, "Scale Min", arg.ScaleMin);
                CH_PROP_NAMED(props, "Scale Max", arg.ScaleMax);
                CH_PROP_NAMED(props, "Graph Size", arg.GraphSize);
            }
            else if constexpr (std::is_same_v<T, PlotHistogramData>) {
                CH_PROP_NAMED(props, "Label", arg.Label);
                CH_SEQUENCE_NAMED(props, "Values", arg.Values);
                CH_PROP_NAMED(props, "Overlay Text", arg.OverlayText);
                CH_PROP_NAMED(props, "Scale Min", arg.ScaleMin);
                CH_PROP_NAMED(props, "Scale Max", arg.ScaleMax);
                CH_PROP_NAMED(props, "Graph Size", arg.GraphSize);
            }
            else if constexpr (std::is_same_v<T, VerticalLayoutGroupData>) {
                CH_PROP_NAMED(props, "Spacing", arg.Spacing);
                CH_PROP_NAMED(props, "Padding", arg.Padding);
            }
        }, Data);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_CONTROL_COMPONENT_H
