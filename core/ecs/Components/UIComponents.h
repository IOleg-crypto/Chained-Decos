#ifndef UI_COMPONENTS_H
#define UI_COMPONENTS_H

#include <raylib.h>
#include <string>

namespace ChainedDecos
{
enum class UIAnchor
{
    TopLeft,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

struct RectTransform
{
    Vector2 position = {0, 0};    // Position relative to anchor
    Vector2 size = {100, 100};    // Size in pixels
    Vector2 pivot = {0.5f, 0.5f}; // Center point for rotation/scaling
    UIAnchor anchor = UIAnchor::MiddleCenter;
    float rotation = 0.0f; // Rotation in degrees
};

struct UIImage
{
    std::string textureName; // Name for resource lookup
    Color tint = WHITE;
    float borderRadius = 0.0f;
    float borderWidth = 0.0f;
    Color borderColor = BLANK;
};

struct UIText
{
    std::string text = "New Text";
    std::string fontName;
    float fontSize = 24.0f;
    float spacing = 2.0f;
    Color color = WHITE;
};

// Button component for standardized interaction
struct UIButton
{
    std::string eventId; // For event handling
    bool isPressed = false;
    bool isHovered = false;
    Color normalColor = GRAY;
    Color hoverColor = LIGHTGRAY;
    Color pressedColor = DARKGRAY;
    float borderRadius = 0.0f;
    float borderWidth = 0.0f;
    Color borderColor = BLACK;

    // Action System
    std::string actionType;
    std::string actionTarget;
};

// ImGui integration component
struct ImGuiComponent
{
    std::string label = "ImGui Widget";
    std::string eventId;
    bool isButton = true;
    bool useSceneTheme = true;
};

// Editor-only component to map back to GameScene index
struct UIElementIndex
{
    int index;
};
} // namespace ChainedDecos

#endif // UI_COMPONENTS_H
