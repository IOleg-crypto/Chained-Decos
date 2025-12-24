#ifndef HIERARCHYPANEL_H
#define HIERARCHYPANEL_H

#include "scene/resources/map/core/SceneLoader.h"
#include <memory>

namespace CHEngine
{
// Forward declaration for EditorLayer
class EditorLayer;

class HierarchyPanel
{
public:
    HierarchyPanel() = default;
    HierarchyPanel(const std::shared_ptr<GameScene> &scene);

    void SetContext(const std::shared_ptr<GameScene> &scene);

    void OnImGuiRender(EditorLayer *layer);

private:
    std::shared_ptr<GameScene> m_Context;
};
} // namespace CHEngine

#endif // HIERARCHYPANEL_H
