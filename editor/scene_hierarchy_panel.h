#ifndef CH_SCENE_HIERARCHY_PANEL_H
#define CH_SCENE_HIERARCHY_PANEL_H

#include "engine/base.h"
#include "engine/entity.h"
#include "engine/scene.h"

namespace CH
{
class SceneHierarchyPanel
{
public:
    SceneHierarchyPanel() = default;
    SceneHierarchyPanel(const Ref<Scene> &context);

    void SetContext(const Ref<Scene> &context);

    void OnImGuiRender();

    Entity GetSelectedEntity() const
    {
        return m_SelectionContext;
    }

private:
    bool DrawEntityNode(Entity entity);

private:
    Ref<Scene> m_Context;
    Entity m_SelectionContext;
};
} // namespace CH

#endif // CH_SCENE_HIERARCHY_PANEL_H
