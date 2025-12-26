#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "editor/EditorTypes.h"
#include <functional>
#include <memory>
#include <string>

class GameScene;

class GameScene;

namespace CHEngine
{
class HierarchyPanel
{
public:
    HierarchyPanel() = default;
    HierarchyPanel(const std::shared_ptr<GameScene> &scene);

    void SetContext(const std::shared_ptr<GameScene> &scene);

    void OnImGuiRender(SelectionType selectionType, int selectedIndex,
                       const std::function<void(SelectionType, int)> &onSelect,
                       const std::function<void()> &onAddModel,
                       const std::function<void(const std::string &)> &onAddUI,
                       const std::function<void(int)> &onDelete);

    bool IsVisible() const
    {
        return m_isVisible;
    }
    void SetVisible(bool visible)
    {
        m_isVisible = visible;
    }

private:
    std::shared_ptr<::GameScene> m_Context;
    bool m_isVisible = true;
};
} // namespace CHEngine

#endif // HIERARCHYPANEL_H
