#ifndef UI_RENDER_SYSTEM_H
#define UI_RENDER_SYSTEM_H

#include "scene/ecs/ECSRegistry.h"
#include "scene/ecs/components/UIComponents.h"
#include <raylib.h>

namespace CHEngine
{
class UIRenderSystem
{
public:
    static void Render(int screenWidth, int screenHeight);
    static void RenderImGui(int screenWidth, int screenHeight, Vector2 offset = {0, 0});

    // Editor support
    static entt::entity PickUIEntity(Vector2 mousePos, int screenWidth, int screenHeight);
    static void DrawSelectionHighlight(entt::entity entity, int screenWidth, int screenHeight);

private:
    static Vector2 CalculateScreenPosition(const RectTransform &transform, int screenWidth,
                                           int screenHeight);
    static Vector2 GetAnchorPosition(UIAnchor anchor, int screenWidth, int screenHeight);
};
} // namespace CHEngine

#endif // UI_RENDER_SYSTEM_H
