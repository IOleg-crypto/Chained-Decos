#ifndef CD_SCENE_ECS_COMPONENTS_UI_COMPONENTS_H
#define CD_SCENE_ECS_COMPONENTS_UI_COMPONENTS_H

#include <cstdint>
#include <raylib.h>
#include <string>

namespace CHEngine
{
enum class UIAnchor : std::uint8_t
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
    bool active = true;    // Whether this UI element is active/visible
};

struct UIImage
{
    std::string textureName; // Name for resource lookup
    std::string texturePath; // Path for loading
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
    std::string eventId; // For event.handling
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

struct UIBackground
{
    Color color = BLANK;
    std::string texturePath;
};

// Editor-only component to map back to GameScene index
struct UIElementIndex
{
    int index;
};
} // namespace CHEngine

#endif // CD_SCENE_ECS_COMPONENTS_UI_COMPONENTS_H
