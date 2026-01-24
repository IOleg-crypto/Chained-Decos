#ifndef CH_SCENE_HIERARCHY_PANEL_H
#define CH_SCENE_HIERARCHY_PANEL_H

#include "panel.h"
#include <unordered_set>

namespace CHEngine
{
class SceneHierarchyPanel : public Panel
{
public:
    SceneHierarchyPanel();
    SceneHierarchyPanel(const Ref<Scene> &context);

    virtual void SetContext(const Ref<Scene> &context) override;
    virtual void OnImGuiRender(bool readOnly = false) override;

    void SetSelectedEntity(Entity entity)
    {
        m_SelectionContext = entity;
    }

    Entity GetSelectedEntity() const
    {
        return m_SelectionContext;
    }

private:
    bool DrawEntityNode(Entity entity);

private:
    Entity m_SelectionContext;
    std::unordered_set<entt::entity> m_DrawnEntities;
};

} // namespace CHEngine

#endif // CH_SCENE_HIERARCHY_PANEL_H
