#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "editor/EditorTypes.h"
#include "scene/resources/map/core/SceneLoader.h"
#include <functional>
#include <memory>

namespace CHEngine
{
class HierarchyPanel
{
public:
    HierarchyPanel() = default;
    HierarchyPanel(const std::shared_ptr<GameScene> &scene);

    void SetContext(const std::shared_ptr<GameScene> &scene);

    void OnImGuiRender(SelectionType selectionType, int selectedIndex,
                       std::function<void(SelectionType, int)> onSelect,
                       std::function<void()> onAddModel,
                       std::function<void(const std::string &)> onAddUI);

private:
    std::shared_ptr<GameScene> m_Context;
};
} // namespace CHEngine

#endif // HIERARCHYPANEL_H
