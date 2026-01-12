#ifndef CH_SCENE_HIERARCHY_PANEL_H
#define CH_SCENE_HIERARCHY_PANEL_H

#include "engine/core/base.h"
#include "engine/core/events.h"
#include "engine/renderer/render.h"
#include "engine/scene/entity.h"
#include "engine/scene/scene.h"

namespace CHEngine
{
class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(const Ref<Scene> &context);

    void SetContext(const Ref<Scene> &context);

    void OnImGuiRender(bool readOnly = false);

    void SetSelectedEntity(Entity entity)
    {
        m_SelectionContext = entity;
    }

    Entity GetSelectedEntity() const
    {
        return m_SelectionContext;
    }

    void SetEventCallback(const EventCallbackFn &callback)
    {
        m_EventCallback = callback;
    }

private:
    bool DrawEntityNode(Entity entity);

private:
    Ref<Scene> m_Context;
    Entity m_SelectionContext;
    std::unordered_set<entt::entity> m_DrawnEntities;
    EventCallbackFn m_EventCallback;
};

} // namespace CHEngine

#endif // CH_SCENE_HIERARCHY_PANEL_H
