#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/core/reflection.h"

namespace CHEngine
{

// Typography & Visual Styles
enum class TextAlignment
{
    Left = 0, Center = 1, Right = 2,
    Top = 0, Bottom = 2
};

enum class CanvasScaleMode : uint8_t
{
    ConstantPixelSize,
    ScaleWithScreenSize,
};

struct CanvasSettings
{
    glm::vec2 ReferenceResolution = {1920.0f, 1080.0f};
    CanvasScaleMode ScaleMode = CanvasScaleMode::ConstantPixelSize;
    float MatchWidthOrHeight = 0.5f;
};

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
    TextAlignment HorizontalAlignment = TextAlignment::Center;
    TextAlignment VerticalAlignment = TextAlignment::Center;

    CH_REFLECT_BEGIN(TextStyle)
        props.Property("Font Name", FontName);
        props.Property("Font Size", FontSize, PropertyMeta(6.0f, 128.0f, 1.0f));
        props.Property("Text Color", TextColor);
        props.Property("Shadow", Shadow);
        if (Shadow)
        {
            props.Property("Shadow Offset", ShadowOffset, PropertyMeta(0.0f, 20.0f, 0.1f));
            props.Property("Shadow Color", ShadowColor);
        }
        props.Property("Letter Spacing", LetterSpacing, PropertyMeta(0.5f, 4.0f, 0.1f));
        props.Property("Line Height", LineHeight, PropertyMeta(0.5f, 3.0f, 0.1f));
        // Alignments as ints for now
        props.Property("H Align", (int&)HorizontalAlignment);
        props.Property("V Align", (int&)VerticalAlignment);
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
        props.Property("BG Color", BackgroundColor);
        props.Property("Hover Color", HoverColor);
        props.Property("Pressed Color", PressedColor);
        props.Property("Rounding", Rounding, PropertyMeta(0.0f, 20.0f, 0.5f));
        props.Property("Border Size", BorderSize, PropertyMeta(0.0f, 10.0f, 0.1f));
        props.Property("Border Color", BorderColor);
        props.Property("Gradient", UseGradient);
        if (UseGradient)
            props.Property("Gradient Color", GradientColor);
        props.Property("Padding", Padding, PropertyMeta(0.0f, 50.0f, 1.0f));
        props.Property("Hover Scale", HoverScale, PropertyMeta(0.8f, 1.5f, 0.05f));
        props.Property("Pressed Scale", PressedScale, PropertyMeta(0.8f, 1.5f, 0.05f));
        props.Property("Transition Speed", TransitionSpeed, PropertyMeta(0.01f, 1.0f, 0.01f));
    CH_REFLECT_END()

    // Runtime state (not serialized)
    struct RuntimeState {
        float AnimationAlpha = 0.0f; // 0 = idle, 1 = hover/pressed
        float CurrentScale = 1.0f;
        Color CurrentColor = {255, 255, 255, 255};
    } State;
};

struct Rectangle
{
    float x, y, width, height;
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

    Rectangle CalculateRect(glm::vec2 viewportSize, glm::vec2 viewportOffset = {0.0f, 0.0f}) const
    {
        glm::vec2 clAnchMin = glm::clamp(AnchorMin, 0.0f, 1.0f);
        glm::vec2 clAnchMax = glm::clamp(AnchorMax, 0.0f, 1.0f);

        glm::vec2 anchorMinPos = {viewportSize.x * clAnchMin.x, viewportSize.y * clAnchMin.y};
        glm::vec2 anchorMaxPos = {viewportSize.x * clAnchMax.x, viewportSize.y * clAnchMax.y};

        glm::vec2 pMin = {anchorMinPos.x + OffsetMin.x, anchorMinPos.y + OffsetMin.y};
        glm::vec2 pMax = {anchorMaxPos.x + OffsetMax.x, anchorMaxPos.y + OffsetMax.y};

        return Rectangle{viewportOffset.x + pMin.x, viewportOffset.y + pMin.y, pMax.x - pMin.x, pMax.y - pMin.y};
    }

    glm::vec2 GetCenter(glm::vec2 viewportSize) const
    {
        Rectangle rect = CalculateRect(viewportSize);
        return {rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f};
    }

    glm::vec2 GetSize(glm::vec2 viewportSize) const
    {
        Rectangle rect = CalculateRect(viewportSize);
        return {rect.width, rect.height};
    }

    CH_REFLECT_BEGIN(RectTransform)
        bool changed = false;
        bool isFill = (AnchorMin.x == 0.0f && AnchorMax.x == 1.0f && AnchorMin.y == 0.0f && AnchorMax.y == 1.0f);
        
        if (props.BeginGroup("Simplified Layout", !isFill))
        {
            if (isFill)
            {
                props.Header("Fill Mode Active");
                props.Property("Padding L/R", OffsetMin.x, PropertyMeta(-1000.0f, 1000.0f, 1.0f)); // Simplified view
                props.Property("Padding T/B", OffsetMin.y, PropertyMeta(-1000.0f, 1000.0f, 1.0f));
            }
            else
            {
                glm::vec2 pos = (OffsetMin + OffsetMax) * 0.5f;
                glm::vec2 size = OffsetMax - OffsetMin;
                
                if (props.Property("Position", pos, PropertyMeta(-2000.0f, 2000.0f, 1.0f)))
                {
                    OffsetMin.x = pos.x - size.x * Pivot.x;
                    OffsetMin.y = pos.y - size.y * Pivot.y;
                    OffsetMax.x = pos.x + size.x * (1.0f - Pivot.x);
                    OffsetMax.y = pos.y + size.y * (1.0f - Pivot.y);
                    changed = true;
                }
                
                if (props.Property("Size", size, PropertyMeta(1.0f, 2000.0f, 1.0f)))
                {
                    OffsetMin.x = pos.x - size.x * Pivot.x;
                    OffsetMin.y = pos.y - size.y * Pivot.y;
                    OffsetMax.x = pos.x + size.x * (1.0f - Pivot.x);
                    OffsetMax.y = pos.y + size.y * (1.0f - Pivot.y);
                    changed = true;
                }
            }
            props.EndGroup();
        }

        if (props.BeginGroup("Advanced Settings", isFill))
        {
            if (props.Property("Anchor Min", AnchorMin)) changed = true;
            if (props.Property("Anchor Max", AnchorMax)) changed = true;
            if (props.Property("Offset Min", OffsetMin, PropertyMeta(-2000.0f, 2000.0f, 1.0f))) changed = true;
            if (props.Property("Offset Max", OffsetMax, PropertyMeta(-2000.0f, 2000.0f, 1.0f))) changed = true;
            if (props.Property("Pivot", Pivot)) changed = true;
            if (props.Property("Rotation", Rotation, PropertyMeta(-360.0f, 360.0f, 1.0f))) changed = true;
            if (props.Property("Scale", Scale, PropertyMeta(0.1f, 10.0f, 0.1f))) changed = true;
            props.EndGroup();
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
        props.Nested("Rect Transform", Transform);
        props.Property("Z Order", ZOrder, PropertyMeta(-1000.0f, 1000.0f, 1.0f));
        props.Property("Active", IsActive);
        props.Property("Hidden", HiddenInHierarchy);
    CH_REFLECT_END()
};

struct ButtonControl
{
    std::string Label = "Button";
    TextStyle Text;
    UIStyle Style;
    bool IsInteractable = true;
    bool PressedThisFrame = false;
    bool IsHovered = false;
    bool IsDown = false;
    bool AutoSize = false;

    ButtonControl() = default;
    ButtonControl(const std::string& label) : Label(label) {}

    CH_REFLECT_BEGIN(ButtonControl)
        props.Property("Label", Label);
        props.Nested("Text Style", Text);
        props.Nested("UI Style", Style);
        props.Property("Interactable", IsInteractable);
        props.Property("Auto Size", AutoSize);
    CH_REFLECT_END()
};

struct PanelControl
{
    UIStyle Style;
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    bool FullScreen = false;

    bool IsHovered = false;
    bool IsDown = false;

    CH_REFLECT_BEGIN(PanelControl)
        props.Nested("UI Style", Style);
        props.Handle("Texture Handle", TextureHandle);
        props.File("Texture Path", TexturePath, "png,jpg,tga");
        props.Property("Full Screen", FullScreen);
    CH_REFLECT_END()
};

struct LabelControl
{
    std::string Text = "Text Label";
    TextStyle Style;
    bool AutoSize = false;

    LabelControl() = default;
    LabelControl(const std::string& text) : Text(text) {}

    CH_REFLECT_BEGIN(LabelControl)
        props.Property("Text", Text);
        props.Nested("Style", Style);
        props.Property("Auto Size", AutoSize);
    CH_REFLECT_END()
};

struct SliderControl
{
    std::string Label = "Slider";
    TextStyle Text;
    float Value = 0.5f;
    float Min = 0.0f;
    float Max = 1.0f;
    bool Changed = false;
    UIStyle Style;

    CH_REFLECT_BEGIN(SliderControl)
        props.Property("Label", Label);
        props.Nested("Text Style", Text);
        props.Property("Value", Value, PropertyMeta(0.0f, 100.0f, 0.1f));
        props.Property("Min", Min, PropertyMeta(-100.0f, 100.0f, 0.1f));
        props.Property("Max", Max, PropertyMeta(-100.0f, 100.0f, 0.1f));
        props.Nested("UI Style", Style);
    CH_REFLECT_END()
};

struct CheckboxControl
{
    std::string Label = "Checkbox";
    TextStyle Text;
    bool Checked = false;
    bool Changed = false;
    UIStyle Style;

    CH_REFLECT_BEGIN(CheckboxControl)
        props.Property("Label", Label);
        props.Nested("Text Style", Text);
        props.Property("Checked", Checked);
        props.Nested("UI Style", Style);
    CH_REFLECT_END()
};

struct InputTextControl
{
    std::string Label = "Input";
    std::string Text = "";
    std::string Placeholder = "Enter text...";
    int MaxLength = 256;
    bool Multiline = false;
    bool ReadOnly = false;
    bool Password = false;
    bool Changed = false;
    std::vector<char> InputBuffer;
    TextStyle Style;
    UIStyle BoxStyle;

    CH_REFLECT_BEGIN(InputTextControl)
        props.Property("Label", Label);
        props.Property("Text", Text);
        props.Property("Placeholder", Placeholder);
        props.Property("MaxLength", MaxLength);
        props.Property("Multiline", Multiline);
        props.Property("ReadOnly", ReadOnly);
        props.Property("Password", Password);
        props.Nested("Text Style", Style);
        props.Nested("Box Style", BoxStyle);
    CH_REFLECT_END()
};

struct ComboBoxControl
{
    std::string Label = "Combo";
    std::vector<std::string> Items = {"Option 1", "Option 2", "Option 3"};
    int SelectedIndex = 0;
    bool Changed = false;
    TextStyle Style;
    UIStyle BoxStyle;

    CH_REFLECT_BEGIN(ComboBoxControl)
        props.Property("Label", Label);
        props.Sequence("Items", Items);
        props.Property("Selected Index", SelectedIndex);
        props.Nested("Style", Style);
        props.Nested("Box Style", BoxStyle);
    CH_REFLECT_END()
};

struct ProgressBarControl
{
    float Progress = 0.5f;
    std::string OverlayText = "";
    bool ShowPercentage = true;
    TextStyle Style;
    UIStyle BarStyle;

    CH_REFLECT_BEGIN(ProgressBarControl)
        props.Property("Progress", Progress);
        props.Property("Overlay Text", OverlayText);
        props.Property("Show Percentage", ShowPercentage);
        props.Nested("Style", Style);
        props.Nested("Bar Style", BarStyle);
    CH_REFLECT_END()
};

struct ImageControl
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    Color TintColor = {255, 255, 255, 255};
    Color BorderColor = {0, 0, 0, 0};
    UIStyle Style;

    bool IsHovered = false;
    bool IsDown = false;

    CH_REFLECT_BEGIN(ImageControl)
        props.Handle("Texture Handle", TextureHandle);
        props.File("Texture Path", TexturePath, "png,jpg,tga");
        props.Property("Tint Color", TintColor);
        props.Property("Border Color", BorderColor);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct ImageButtonControl
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    std::string Label = "ImageButton";
    Color TintColor = {255, 255, 255, 255};
    Color BackgroundColor = {0, 0, 0, 0};
    int FramePadding = -1;
    bool PressedThisFrame = false;
    UIStyle Style;

    CH_REFLECT_BEGIN(ImageButtonControl)
        props.Handle("Texture Handle", TextureHandle);
        props.File("Texture Path", TexturePath, "png,jpg,tga");
        props.Property("Label", Label);
        props.Property("Tint Color", TintColor);
        props.Property("Background Color", BackgroundColor);
        props.Property("Frame Padding", FramePadding);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct SeparatorControl
{
    float Thickness = 1.0f;
    Color LineColor = {127, 127, 127, 255};

    CH_REFLECT_BEGIN(SeparatorControl)
        props.Property("Thickness", Thickness);
        props.Property("Line Color", LineColor);
    CH_REFLECT_END()
};

struct RadioButtonControl
{
    std::string Label = "RadioGroup";
    std::vector<std::string> Options = {"Option 1", "Option 2", "Option 3"};
    int SelectedIndex = 0;
    bool Changed = false;
    bool Horizontal = false;
    TextStyle Style;

    CH_REFLECT_BEGIN(RadioButtonControl)
        props.Property("Label", Label);
        props.Sequence("Options", Options);
        props.Property("Selected Index", SelectedIndex);
        props.Property("Horizontal", Horizontal);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct ColorPickerControl
{
    std::string Label = "Color";
    Color SelectedColor = {255, 255, 255, 255};
    bool ShowAlpha = true;
    bool ShowPicker = true;
    bool Changed = false;
    UIStyle Style;

    CH_REFLECT_BEGIN(ColorPickerControl)
        props.Property("Label", Label);
        props.Property("Color", SelectedColor);
        props.Property("Show Alpha", ShowAlpha);
        props.Property("Show Picker", ShowPicker);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct DragFloatControl
{
    std::string Label = "DragFloat";
    float Value = 0.0f;
    float Speed = 0.1f;
    float Min = 0.0f;
    float Max = 100.0f;
    std::string Format = "%.3f";
    bool Changed = false;
    TextStyle Style;
    UIStyle BoxStyle;

    CH_REFLECT_BEGIN(DragFloatControl)
        props.Property("Label", Label);
        props.Property("Value", Value);
        props.Property("Speed", Speed);
        props.Property("Min", Min);
        props.Property("Max", Max);
        props.Nested("Style", Style);
        props.Nested("Box Style", BoxStyle);
    CH_REFLECT_END()
};

struct DragIntControl
{
    std::string Label = "DragInt";
    int Value = 0;
    float Speed = 1.0f;
    int Min = 0;
    int Max = 100;
    std::string Format = "%d";
    bool Changed = false;
    TextStyle Style;
    UIStyle BoxStyle;

    CH_REFLECT_BEGIN(DragIntControl)
        props.Property("Label", Label);
        props.Property("Value", Value);
        props.Property("Speed", Speed);
        props.Property("Min", Min);
        props.Property("Max", Max);
        props.Nested("Style", Style);
        props.Nested("Box Style", BoxStyle);
    CH_REFLECT_END()
};

struct TreeNodeControl
{
    std::string Label = "TreeNode";
    bool IsOpen = false;
    bool DefaultOpen = false;
    bool IsLeaf = false;
    TextStyle Style;

    CH_REFLECT_BEGIN(TreeNodeControl)
        props.Property("Label", Label);
        props.Property("Is Open", IsOpen);
        props.Property("Default Open", DefaultOpen);
        props.Property("Is Leaf", IsLeaf);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct TabBarControl
{
    std::string Label = "TabBar";
    bool Reorderable = true;
    bool AutoSelectNewTabs = true;
    UIStyle Style;

    CH_REFLECT_BEGIN(TabBarControl)
        props.Property("Label", Label);
        props.Property("Reorderable", Reorderable);
        props.Property("Auto Select New Tabs", AutoSelectNewTabs);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct TabItemControl
{
    std::string Label = "Tab";
    bool IsOpen = true;
    bool Selected = false;
    TextStyle Style;

    CH_REFLECT_BEGIN(TabItemControl)
        props.Property("Label", Label);
        props.Property("Is Open", IsOpen);
        props.Property("Selected", Selected);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct CollapsingHeaderControl
{
    std::string Label = "Header";
    bool IsOpen = false;
    bool DefaultOpen = false;
    TextStyle Style;

    CH_REFLECT_BEGIN(CollapsingHeaderControl)
        props.Property("Label", Label);
        props.Property("Is Open", IsOpen);
        props.Property("Default Open", DefaultOpen);
        props.Nested("Style", Style);
    CH_REFLECT_END()
};

struct PlotLinesControl
{
    std::string Label = "Plot";
    std::vector<float> Values = {0.0f, 0.5f, 1.0f, 0.5f, 0.0f};
    std::string OverlayText = "";
    float ScaleMin = 0.0f;
    float ScaleMax = 1.0f;
    glm::vec2 GraphSize = {0, 80};
    TextStyle Style;
    UIStyle BoxStyle;

    CH_REFLECT_BEGIN(PlotLinesControl)
        props.Property("Label", Label);
        props.Property("Values", Values);
        props.Property("Overlay Text", OverlayText);
        props.Property("Scale Min", ScaleMin);
        props.Property("Scale Max", ScaleMax);
        props.Property("Graph Size", GraphSize);
        props.Nested("Style", Style);
        props.Nested("Box Style", BoxStyle);
    CH_REFLECT_END()
};

struct PlotHistogramControl
{
    std::string Label = "Histogram";
    std::vector<float> Values = {0.2f, 0.5f, 0.8f, 0.4f, 0.6f};
    std::string OverlayText = "";
    float ScaleMin = 0.0f;
    float ScaleMax = 1.0f;
    glm::vec2 GraphSize = {0, 80};
    TextStyle Style;
    UIStyle BoxStyle;

    CH_REFLECT_BEGIN(PlotHistogramControl)
        props.Property("Label", Label);
        props.Property("Values", Values);
        props.Property("Overlay Text", OverlayText);
        props.Property("Scale Min", ScaleMin);
        props.Property("Scale Max", ScaleMax);
        props.Property("Graph Size", GraphSize);
        props.Nested("Style", Style);
        props.Nested("Box Style", BoxStyle);
    CH_REFLECT_END()
};

struct VerticalLayoutGroup
{
    float Spacing = 10.0f;
    glm::vec2 Padding = {10, 10};

    CH_REFLECT_BEGIN(VerticalLayoutGroup)
        props.Property("Spacing", Spacing);
        props.Property("Padding", Padding);
    CH_REFLECT_END()
};

} // namespace CHEngine

#endif // CH_CONTROL_COMPONENT_H
