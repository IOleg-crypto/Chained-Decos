#include "editor/panels/EditorPanel.h"
#include "scene/resources/map/GameScene.h"
#include <entt/entt.hpp>
#include <functional>
#include <imgui.h>
#include <memory>
#include <string>

namespace CHEngine
{
class Scene;
class SelectionManager;
class CommandHistory;

/**
 * @brief Panel for inspecting and editing entity/object properties
 */
class InspectorPanel : public EditorPanel
{
public:
    InspectorPanel() = default;
    InspectorPanel(SelectionManager *selection, CommandHistory *history);
    ~InspectorPanel() = default;

    // --- Panel Lifecycle ---
public:
    virtual void OnImGuiRender() override;

    // --- Member Variables ---
private:
    SelectionManager *m_SelectionManager = nullptr;
    CommandHistory *m_CommandHistory = nullptr;

    std::function<void(int, const MapObjectData &, const MapObjectData &)> m_onPropertyChange;
    std::function<void(const std::string &)> m_onSkyboxSelected;
};
} // namespace CHEngine
