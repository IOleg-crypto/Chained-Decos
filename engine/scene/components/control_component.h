#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include <glm/glm.hpp>
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

#include "engine/scene/reflect.h"

namespace CHEngine
{
using vec2 = glm::vec2;

// Screen-space rectangle (absolute pixel coordinates)
struct Rect
{
    vec2 Min; // Top-left
    vec2 Max; // Bottom-right

    vec2 Size() const { return Max - Min; }
    vec2 Center() const { return (Min + Max) * 0.5f; }

    bool Contains(vec2 point) const
    {
        return point.x >= Min.x && point.x <= Max.x && point.y >= Min.y && point.y <= Max.y;
    }
};

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
    vec2 AnchorMin = {0.5f, 0.5f};
    vec2 AnchorMax = {0.5f, 0.5f};
    vec2 OffsetMin = {-50.0f, -20.0f};
    vec2 OffsetMax = {50.0f, 20.0f};
    vec2 Pivot = {0.5f, 0.5f};
    float Rotation = 0.0f;
    vec2 Scale = {1.0f, 1.0f};

    Rect CalculateRect(vec2 viewportSize, vec2 viewportOffset = {0.0f, 0.0f}) const
    {
        // 1. Calculate the box defined by anchors (clamped to 0..1)
        vec2 clAnchMin = glm::clamp(AnchorMin, vec2(0.0f), vec2(1.0f));
        vec2 clAnchMax = glm::clamp(AnchorMax, vec2(0.0f), vec2(1.0f));

        vec2 anchorMinPos = {viewportSize.x * clAnchMin.x, viewportSize.y * clAnchMin.y};
        vec2 anchorMaxPos = {viewportSize.x * clAnchMax.x, viewportSize.y * clAnchMax.y};

        // 2. Add offsets (absolute pixels)
        vec2 pMin = anchorMinPos + OffsetMin;
        vec2 pMax = anchorMaxPos + OffsetMax;

        // 3. Apply Pivot (for fixed-size elements where AnchorMin == AnchorMax)
        if (AnchorMin == AnchorMax)
        {
            // The OffsetMin/Max are already defined relative to the anchors.
            // Pivot mostly affects rotation and scaling center in a full UI system,
            // but for absolute Rect calculation, pMin/pMax are usually sufficient
            // if they define the final box.
        }

        return Rect{{viewportOffset.x + pMin.x, viewportOffset.y + pMin.y},
                    {viewportOffset.x + pMax.x, viewportOffset.y + pMax.y}};
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


BEGIN_REFLECT(RectTransform)
    PROPERTY(vec2, AnchorMin, "Anchor Min")
    PROPERTY(vec2, AnchorMax, "Anchor Max")
    PROPERTY(vec2, OffsetMin, "Offset Min")
    PROPERTY(vec2, OffsetMax, "Offset Max")
    PROPERTY(vec2, Pivot, "Pivot")
    PROPERTY(float, Rotation, "Rotation")
    PROPERTY(vec2, Scale, "Scale")
END_REFLECT()

BEGIN_REFLECT(ControlComponent)
    PROPERTY(int, ZOrder, "Z Order")
    PROPERTY(bool, IsActive, "Active")
END_REFLECT()

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

BEGIN_REFLECT(LabelControl)
    PROPERTY(std::string, Text, "Text")
    // Note: Style is complex, but we can reflect its basic fields if needed
END_REFLECT()

BEGIN_REFLECT(ButtonControl)
    PROPERTY(std::string, Label, "Label")
    PROPERTY(bool, IsInteractable, "Interactable")
END_REFLECT()

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

// Layouts
struct VerticalLayoutGroup
{
    float Spacing = 10.0f;
    Vector2 Padding = {10, 10};
    // Future: Alignment
};

} // namespace CHEngine

#endif // CH_CONTROL_COMPONENT_H
