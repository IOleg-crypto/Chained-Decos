#ifndef INSPECTOR_PANEL_H
#define INSPECTOR_PANEL_H

#include "scene/resources/map/GameScene.h"
#include <entt/entt.hpp>
#include <functional>
#include <imgui.h>
#include <memory>
#include <string>

namespace CHEngine
{
class Scene;

/**
 * @brief Panel for inspecting and editing entity/object properties
 */
class InspectorPanel
{
public:
    InspectorPanel() = default;
    ~InspectorPanel() = default;

    // --- Panel Lifecycle ---
public:
    void OnImGuiRender(const std::shared_ptr<GameScene> &scene, int selectedObjectIndex,
                       MapObjectData *selectedEntity);
    void OnImGuiRender(const std::shared_ptr<GameScene> &scene, UIElementData *selectedElement);
    void OnImGuiRender(const std::shared_ptr<Scene> &scene, entt::entity entity);

    // --- Callbacks & Configuration ---
public:
    void SetPropertyChangeCallback(
        std::function<void(int, const MapObjectData &, const MapObjectData &)> cb);
    void SetSkyboxCallback(std::function<void(const std::string &)> cb);

    bool IsVisible() const;
    void SetVisible(bool visible);

    // --- Private Drawing Helpers ---
private:
    void DrawSceneSettings(const std::shared_ptr<GameScene> &scene);
    void DrawComponents(MapObjectData *entity);
    void DrawUIComponents(UIElementData *element);
    void DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                         float columnWidth = 100.0f);
    void DrawEntityComponents(const std::shared_ptr<Scene> &scene, entt::entity entity);

    // --- Member Variables ---
private:
    std::function<void(int, const MapObjectData &, const MapObjectData &)> m_onPropertyChange;
    std::function<void(const std::string &)> m_onSkyboxSelected;
    bool m_isVisible = true;
};
} // namespace CHEngine

#endif // INSPECTOR_PANEL_H
