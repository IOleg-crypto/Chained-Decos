#ifndef CH_SCENE_HIERARCHY_PANEL_H
#define CH_SCENE_HIERARCHY_PANEL_H

#include "panel.h"
#include "unordered_set"

namespace CHEngine
{
class SceneHierarchyPanel : public Panel
{
public:
    SceneHierarchyPanel();
    SceneHierarchyPanel(const std::shared_ptr<Scene>& context);

    virtual void OnImGuiRender(bool readOnly = false) override;

private:
    entt::entity DrawEntityNodeRecursive(Entity entity);
    void DrawComponents(Entity entity);
    void DrawContextMenu();
    const char* GetEntityIcon(Entity entity);

private:
    std::unordered_set<entt::entity> m_DrawnEntities;
};

} // namespace CHEngine

#endif // CH_SCENE_HIERARCHY_PANEL_H
