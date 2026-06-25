#ifndef CH_CONTROL_COMPONENT_H
#define CH_CONTROL_COMPONENT_H

#include "engine/reflection/reflection.h"
#include "engine/reflection/reflection_rfl.h"
#include "engine/graphics/ui/ui_style.h"
#include "engine/graphics/ui/ui_data_components.h"

namespace Chained
{
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


struct UIControlComponent
{
    UIStyle BoxStyle;
    TextStyle TextStyle;
    ControlData Data = std::monostate{};

    bool IsHovered = false;
    bool IsDown = false;
    bool PressedThisFrame = false;
    bool ValueChanged = false;

    static const char* GetStaticName() { return "UIControlComponent"; }
};

CH_MARK_RFL(UIControlComponent);

} // namespace Chained

#endif // CH_CONTROL_COMPONENT_H
