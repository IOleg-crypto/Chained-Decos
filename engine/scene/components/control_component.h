#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/core/reflection.h"
#include "engine/core/reflection_rfl.h"
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

    static const char* GetStaticName() { return "TextStyle"; }
};

CH_MARK_RFL(TextStyle);

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

    // Runtime state (not serialized)
    struct RuntimeState {
        float AnimationAlpha = 0.0f; // 0 = idle, 1 = hover/pressed
        float CurrentScale = 1.0f;
        Color CurrentColor = {255, 255, 255, 255};
    } State;
};

CH_MARK_RFL(UIStyle);


struct RectTransform
{
    glm::vec2 AnchorMin = {0.5f, 0.5f};
    glm::vec2 AnchorMax = {0.5f, 0.5f};
    glm::vec2 OffsetMin = {-50.0f, -20.0f};
    glm::vec2 OffsetMax = {50.0f, 20.0f};
    glm::vec2 Pivot = {0.5f, 0.5f};
    float Rotation = 0.0f;
    glm::vec2 Scale = {1.0f, 1.0f};

    static const char* GetStaticName() { return "RectTransform"; }
};

CH_MARK_RFL(RectTransform);

struct ControlComponent
{
    RectTransform Transform;
    int32_t ZOrder = 0;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    static const char* GetStaticName() { return "ControlComponent"; }
};

CH_MARK_RFL(ControlComponent);

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

    static const char* GetStaticName() { return "WidgetComponent"; }
};

CH_MARK_RFL(WidgetComponent);

} // namespace CHEngine

#endif // CH_CONTROL_COMPONENT_H
