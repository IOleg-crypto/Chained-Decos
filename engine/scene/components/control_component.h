#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include <glm/glm.hpp>
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

namespace CHEngine
{
using vec2 = glm::vec2;

// Typography & Visual Styles
enum class TextAlignment
{
    Left = 0,
    Center,
    Right
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
    vec2 ReferenceResolution = {1920.0f, 1080.0f};
    CanvasScaleMode ScaleMode = CanvasScaleMode::ScaleWithScreenSize;
    float MatchWidthOrHeight = 0.5f; // 0 = match width, 1 = match height, 0.5 = blend
};

struct TextStyle
{
    std::string FontPath = "";
    float FontSize = 18.0f;
    Color TextColor = WHITE;
    bool Shadow = false;
    float ShadowOffset = 2.0f;
    Color ShadowColor = BLACK;
    float LetterSpacing = 1.0f;
    TextAlignment Alignment = TextAlignment::Center;
};

enum class ButtonAction : uint8_t
{
    None = 0,
    LoadScene,
    Quit
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
};

struct RectTransform
{
    vec2 AnchorMin = {0.5f, 0.5f};
    vec2 AnchorMax = {0.5f, 0.5f};
    vec2 OffsetMin = {-50.0f, -20.0f};
    vec2 OffsetMax = {50.0f, 20.0f};
    vec2 Pivot = {0.5f, 0.5f};
    vec2 RectCoordinates = {0.0f, 0.0f};
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

    ButtonAction Action = ButtonAction::None;
    std::string TargetScene = "";

    // Internal state
    bool IsHovered = false;
    bool IsDown = false;

    ButtonControl() = default;
    ButtonControl(const std::string &label) : Label(label)
    {
    }
};

struct PanelControl
{
    UIStyle Style;
    std::string TexturePath = ""; // For 2D backgrounds
    std::shared_ptr<class TextureAsset> Texture = nullptr;

    bool FullScreen = false;
};

struct LabelControl
{
    std::string Text = "Text Label";
    TextStyle Style;

    LabelControl() = default;
    LabelControl(const std::string &text) : Text(text)
    {
    }
};

struct SliderControl
{
    float Value = 0.5f;
    float Min = 0.0f;
    float Max = 1.0f;
    bool Changed = false;

    UIStyle Style;
};

struct CheckboxControl
{
    bool Checked = false;
    bool Changed = false;

    UIStyle Style;
};

// Layouts
struct VerticalLayoutGroup
{
    float Spacing = 10.0f;
    Vector2 Padding = {10, 10};
    // Future: Alignment
};

} // namespace CHEngine

#endif // CH_CONTROL_COMPONENT_H
