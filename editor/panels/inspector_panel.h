#ifndef CH_INSPECTOR_PANEL_H
#define CH_INSPECTOR_PANEL_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/renderer/render.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class InspectorPanel
{
public:
    InspectorPanel() = default;

public:
    void OnImGuiRender(Scene *scene, Entity entity, bool readOnly = false);

private:
    void DrawComponents(Entity entity);
    void DrawTagComponent(Entity entity);
    void DrawTransformComponent(Entity entity);
    void DrawModelComponent(Entity entity);
    void DrawColliderComponent(Entity entity);
    void DrawRigidBodyComponent(Entity entity);
    void DrawSpawnComponent(Entity entity);
    void DrawPlayerComponent(Entity entity);
    void DrawMaterialComponent(Entity entity);
    void DrawPointLightComponent(Entity entity);
    void DrawAudioComponent(Entity entity);
    void DrawHierarchyComponent(Entity entity);
    void DrawCSharpScriptComponent(Entity entity);

    void DrawAddComponentPopup(Entity entity);
};
} // namespace CHEngine

#endif // CH_INSPECTOR_PANEL_H
