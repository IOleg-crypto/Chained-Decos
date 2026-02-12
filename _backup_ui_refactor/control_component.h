#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/graphics/asset.h"
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <vector>

namespace CHEngine
{

// Typography & Visual Styles
enum class TextAlignment
{
    Left = 0,
    Center = 1,
    Right = 2,
    Top = 0,
    Bottom = 2
};

// Canvas scaling modes for Reference Resolution system
enum class CanvasScaleMode : uint8_t
{
    ConstantPixelSize,   // No scaling, pixel-perfect at any resolution
    ScaleWithScreenSize, // Scale proportionally based on reference resolution
};

// Scene-wide canvas settings for UI scaling
struct CanvasSettings
{
    Vector2 ReferenceResolution = {1920.0f, 1080.0f};
    CanvasScaleMode ScaleMode = CanvasScaleMode::ConstantPixelSize; // No scaling, anchors relative to viewport
    float MatchWidthOrHeight = 0.5f; // Only used with ScaleWithScreenSize
};

struct TextStyle
{
    std::string FontName = "Default";
    float FontSize = 18.0f;
    Color TextColor = WHITE;
    bool Shadow = false;
    float ShadowOffset = 2.0f;
    Color ShadowColor = BLACK;
    float LetterSpacing = 1.0f;
    float LineHeight = 1.2f;
    TextAlignment HorizontalAlignment = TextAlignment::Center;
    TextAlignment VerticalAlignment = TextAlignment::Center;
};

struct UIStyle
{
    Color BackgroundColor = {40, 40, 40, 255};
    Color HoverColor = {60, 60, 60, 255};
    Color PressedColor = {30, 30, 30, 255};

    float Rounding = 4.0f;
    float BorderSize = 0.0f;
    Color BorderColor = WHITE;

    bool UseGradient = false;
    Color GradientColor = {20, 20, 20, 255};

    float Padding = 4.0f;
    
    float HoverScale = 1.0f;
    float PressedScale = 1.0f;
    float TransitionSpeed = 0.1f;
};

struct RectTransform
{
    
    Vector2 AnchorMin = {0.5f, 0.5f};
    Vector2 AnchorMax = {0.5f, 0.5f};
    Vector2 OffsetMin = {-50.0f, -20.0f};
    Vector2 OffsetMax = {50.0f, 20.0f};
    Vector2 Pivot = {0.5f, 0.5f};
    float Rotation = 0.0f;
    Vector2 Scale = {1.0f, 1.0f};

    Rectangle CalculateRect(Vector2 viewportSize, Vector2 viewportOffset = {0.0f, 0.0f}) const
    {
        // 1. Calculate the box defined by anchors (clamped to 0..1)
        Vector2 clAnchMin = { Clamp(AnchorMin.x, 0.0f, 1.0f), Clamp(AnchorMin.y, 0.0f, 1.0f) };
        Vector2 clAnchMax = { Clamp(AnchorMax.x, 0.0f, 1.0f), Clamp(AnchorMax.y, 0.0f, 1.0f) };

        Vector2 anchorMinPos = {viewportSize.x * clAnchMin.x, viewportSize.y * clAnchMin.y};
        Vector2 anchorMaxPos = {viewportSize.x * clAnchMax.x, viewportSize.y * clAnchMax.y};

        // 2. Add offsets (absolute pixels)
        Vector2 pMin = { anchorMinPos.x + OffsetMin.x, anchorMinPos.y + OffsetMin.y };
        Vector2 pMax = { anchorMaxPos.x + OffsetMax.x, anchorMaxPos.y + OffsetMax.y };

        // 3. Return generic Raylib Rectangle (x, y, w, h)
        return Rectangle{
            viewportOffset.x + pMin.x, 
            viewportOffset.y + pMin.y, 
            pMax.x - pMin.x, 
            pMax.y - pMin.y
        };
    }

    // New helper to get center position in viewport space
    Vector2 GetCenter(Vector2 viewportSize) const
    {
        Rectangle rect = CalculateRect(viewportSize);
        return { rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f };
    }

    Vector2 GetSize(Vector2 viewportSize) const
    {
        Rectangle rect = CalculateRect(viewportSize);
        return { rect.width, rect.height };
    }
};

// Base Component
struct ControlComponent
{
    RectTransform Transform;
    int32_t ZOrder = 0;
    bool IsActive = true;
    bool HiddenInHierarchy = false;

    ControlComponent() = default;
};

// --- Unified Specialized Widgets ---

struct ButtonControl
{
    std::string Label = "Button";
    TextStyle Text;
    UIStyle Style;

    bool IsInteractable = true;
    bool PressedThisFrame = false;

    // Internal state
    bool IsHovered = false;
    bool IsDown = false;

    bool AutoSize = false;

    ButtonControl() = default;
    ButtonControl(const std::string &label) : Label(label)
    {
    }
};

struct PanelControl
{
    UIStyle Style;
    AssetHandle TextureHandle = 0;
    std::string TexturePath = ""; // For 2D backgrounds
    std::shared_ptr<class TextureAsset> Texture = nullptr;

    bool FullScreen = false;
};

struct LabelControl
{
    std::string Text = "Text Label";
    TextStyle Style;

    bool AutoSize = false;

    LabelControl() = default;
    LabelControl(const std::string &text) : Text(text)
    {
    }
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
    float Progress = 0.5f; // 0.0 - 1.0
    std::string OverlayText = "";
    bool ShowPercentage = true;
    
    TextStyle Style;
    UIStyle BarStyle;
};

// === Visual Widgets ===

struct ImageControl
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    Color TintColor = {255, 255, 255, 255};
    Color BorderColor = {0, 0, 0, 0};
    
    UIStyle Style;
};

struct ImageButtonControl
{
    AssetHandle TextureHandle = 0;
    std::string TexturePath = "";
    std::string Label = "ImageButton";
    Color TintColor = {255, 255, 255, 255};
    Color BackgroundColor = {0, 0, 0, 0};
    int FramePadding = -1;  // -1 = use default
    bool PressedThisFrame = false;
    
    UIStyle Style;
};

struct SeparatorControl
{
    float Thickness = 1.0f;
    Color LineColor = {127, 127, 127, 255};
};

// === Input Widgets ===

struct RadioButtonControl
{
    std::string Label = "RadioGroup";
    std::vector<std::string> Options = {"Option 1", "Option 2", "Option 3"};
    int SelectedIndex = 0;
    bool Changed = false;
    bool Horizontal = false;  // Layout direction
    
    TextStyle Style;
};

struct ColorPickerControl
{
    std::string Label = "Color";
    Color SelectedColor = {255, 255, 255, 255};
    bool ShowAlpha = true;
    bool ShowPicker = true;  // vs just color edit
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

// === Structural Widgets ===

struct TreeNodeControl
{
    std::string Label = "TreeNode";
    bool IsOpen = false;
    bool DefaultOpen = false;
    bool IsLeaf = false;  // No arrow
    
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

// === Data Visualization ===

struct PlotLinesControl
{
    std::string Label = "Plot";
    std::vector<float> Values = {0.0f, 0.5f, 1.0f, 0.5f, 0.0f};
    std::string OverlayText = "";
    float ScaleMin = 0.0f;
    float ScaleMax = 1.0f;
    Vector2 GraphSize = {0, 80};  // 0 = auto width
    
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
    Vector2 GraphSize = {0, 80};
    
    TextStyle Style;
    UIStyle BoxStyle;
};

// Layouts
struct VerticalLayoutGroup
{
    float Spacing = 10.0f;
    Vector2 Padding = {10, 10};
};

} // namespace CHEngine

#endif // CH_CONTROL_COMPONENT_H
