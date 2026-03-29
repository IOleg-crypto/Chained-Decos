#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/core/assets/asset.h"
#include "engine/core/base.h"
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

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
    float LetterSpacing = 1.0f;
    float LineHeight = 1.2f;
    TextAlignment HorizontalAlignment = TextAlignment::Center;
    TextAlignment VerticalAlignment = TextAlignment::Center;

    template <typename Archive>
    static void Serialize(Archive& archive, TextStyle& style)
    {
        archive.Property("FontName", style.FontName)
            .Property("FontSize", style.FontSize)
            .Property("TextColor", style.TextColor)
            .Property("Shadow", style.Shadow)
            .Property("LetterSpacing", style.LetterSpacing)
            .Property("LineHeight", style.LineHeight)
            .Property("HorizontalAlignment", (int&)style.HorizontalAlignment)
            .Property("VerticalAlignment", (int&)style.VerticalAlignment);

        if (style.Shadow)
        {
            archive.Property("ShadowOffset", style.ShadowOffset)
                .Property("ShadowColor", style.ShadowColor);
        }
    }
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

    template <typename Archive>
    static void Serialize(Archive& archive, UIStyle& style)
    {
        archive.Property("BackgroundColor", style.BackgroundColor)
            .Property("HoverColor", style.HoverColor)
            .Property("PressedColor", style.PressedColor)
            .Property("Rounding", style.Rounding)
            .Property("BorderSize", style.BorderSize)
            .Property("BorderColor", style.BorderColor)
            .Property("UseGradient", style.UseGradient)
            .Property("Padding", style.Padding)
            .Property("HoverScale", style.HoverScale)
            .Property("PressedScale", style.PressedScale)
            .Property("TransitionSpeed", style.TransitionSpeed);

        if (style.UseGradient)
        {
            archive.Property("GradientColor", style.GradientColor);
        }
    }

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

    template <typename Archive>
    static void Serialize(Archive& archive, RectTransform& transform)
    {
        archive.Property("AnchorMin", transform.AnchorMin)
            .Property("AnchorMax", transform.AnchorMax)
            .Property("OffsetMin", transform.OffsetMin)
            .Property("OffsetMax", transform.OffsetMax)
            .Property("Pivot", transform.Pivot)
            .Property("Rotation", transform.Rotation)
            .Property("Scale", transform.Scale);
    }
};

struct ControlComponent
{
    RectTransform Transform;
    int32_t ZOrder = 0;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    ControlComponent() = default;

    static const char* GetStaticName() { return "ControlComponent"; }

    template <typename Archive>
    static void Serialize(Archive& archive, ControlComponent& component)
    {
        archive.Property("Transform", component.Transform)
            .Property("ZOrder", component.ZOrder)
            .Property("IsActive", component.IsActive)
            .Property("HiddenInHierarchy", component.HiddenInHierarchy);
    }
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

    static const char* GetStaticName() { return "ButtonControl"; }

    template <typename Archive>
    static void Serialize(Archive& archive, ButtonControl& component)
    {
        archive.Property("Label", component.Label)
            .Property("TextStyle", component.Text)
            .Property("UIStyle", component.Style)
            .Property("IsInteractable", component.IsInteractable)
            .Property("AutoSize", component.AutoSize);
    }
};

struct PanelControl
{
    UIStyle Style;
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    std::shared_ptr<class TextureAsset> Texture = nullptr;
    bool FullScreen = false;

    bool IsHovered = false;
    bool IsDown = false;
};

struct LabelControl
{
    std::string Text = "Text Label";
    TextStyle Style;
    bool AutoSize = false;

    LabelControl() = default;
    LabelControl(const std::string& text) : Text(text) {}
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
};

struct CheckboxControl
{
    std::string Label = "Checkbox";
    TextStyle Text;
    bool Checked = false;
    bool Changed = false;
    UIStyle Style;
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
};

struct ComboBoxControl
{
    std::string Label = "Combo";
    std::vector<std::string> Items = {"Option 1", "Option 2", "Option 3"};
    int SelectedIndex = 0;
    bool Changed = false;
    TextStyle Style;
    UIStyle BoxStyle;
};

struct ProgressBarControl
{
    float Progress = 0.5f;
    std::string OverlayText = "";
    bool ShowPercentage = true;
    TextStyle Style;
    UIStyle BarStyle;
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

    template <typename Archive>
    static void Serialize(Archive& archive, ImageControl& component)
    {
        archive.Property("TexturePath", component.TexturePath)
            .Property("TintColor", component.TintColor)
            .Property("BorderColor", component.BorderColor)
            .Property("Style", component.Style);
    }
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
};

struct SeparatorControl
{
    float Thickness = 1.0f;
    Color LineColor = {127, 127, 127, 255};
};

struct RadioButtonControl
{
    std::string Label = "RadioGroup";
    std::vector<std::string> Options = {"Option 1", "Option 2", "Option 3"};
    int SelectedIndex = 0;
    bool Changed = false;
    bool Horizontal = false;
    TextStyle Style;
};

struct ColorPickerControl
{
    std::string Label = "Color";
    Color SelectedColor = {255, 255, 255, 255};
    bool ShowAlpha = true;
    bool ShowPicker = true;
    bool Changed = false;
    UIStyle Style;
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
};

struct TreeNodeControl
{
    std::string Label = "TreeNode";
    bool IsOpen = false;
    bool DefaultOpen = false;
    bool IsLeaf = false;
    TextStyle Style;
};

struct TabBarControl
{
    std::string Label = "TabBar";
    bool Reorderable = true;
    bool AutoSelectNewTabs = true;
    UIStyle Style;
};

struct TabItemControl
{
    std::string Label = "Tab";
    bool IsOpen = true;
    bool Selected = false;
    TextStyle Style;
};

struct CollapsingHeaderControl
{
    std::string Label = "Header";
    bool IsOpen = false;
    bool DefaultOpen = false;
    TextStyle Style;
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
};

struct VerticalLayoutGroup
{
    float Spacing = 10.0f;
    glm::vec2 Padding = {10, 10};
};

} // namespace CHEngine

#endif // CH_CONTROL_COMPONENT_H
