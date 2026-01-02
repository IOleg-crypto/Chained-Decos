#ifndef CD_SCENE_ECS_SYSTEMS_UI_RENDER_SYSTEM_H
#define CD_SCENE_ECS_SYSTEMS_UI_RENDER_SYSTEM_H

#include "scene/ecs/components/ui_components.h"
#include "scene/ecs/ecs_registry.h"
#include <raylib.h>


namespace CHEngine
{
class UIRenderSystem
{
public:
    static void Render(entt::registry &registry, int screenWidth, int screenHeight);
    static void RenderHUD(entt::registry &registry, int screenWidth, int screenHeight);
    static void RenderImGui(entt::registry &registry, int screenWidth, int screenHeight,
                            Vector2 offset = {0, 0});

    // Editor support
    static entt::entity PickUIEntity(entt::registry &registry, Vector2 mousePos, int screenWidth,
                                     int screenHeight);
    static void DrawSelectionHighlight(entt::entity entity, int screenWidth, int screenHeight);

private:
    static Vector2 CalculateScreenPosition(const RectTransform &transform, int screenWidth,
                                           int screenHeight);
    static Vector2 GetAnchorPosition(UIAnchor anchor, int screenWidth, int screenHeight);
};
} // namespace CHEngine

#endif // CD_SCENE_ECS_SYSTEMS_UI_RENDER_SYSTEM_H
