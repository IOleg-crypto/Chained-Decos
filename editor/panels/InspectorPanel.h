#pragma once

#include "scene/resources/map/GameScene.h"
#include <functional>
#include <imgui.h>
#include <memory>
#include <string>

namespace CHEngine
{
class InspectorPanel
{
public:
    InspectorPanel() = default;

    void OnImGuiRender(const std::shared_ptr<GameScene> &scene, int selectedObjectIndex,
                       MapObjectData *selectedEntity);
    void OnImGuiRender(const std::shared_ptr<GameScene> &scene, UIElementData *selectedElement);

    void SetPropertyChangeCallback(
        std::function<void(int, const MapObjectData &, const MapObjectData &)> cb)
    {
        m_onPropertyChange = cb;
    }

    void SetSkyboxCallback(std::function<void(const std::string &)> cb)
    {
        m_onSkyboxSelected = cb;
    }

    bool IsVisible() const
    {
        return m_isVisible;
    }
    void SetVisible(bool visible)
    {
        m_isVisible = visible;
    }

private:
    void DrawSceneSettings(const std::shared_ptr<GameScene> &scene);
    void DrawComponents(MapObjectData *entity);
    void DrawUIComponents(UIElementData *element);
    void DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                         float columnWidth = 100.0f);

    std::function<void(int, const MapObjectData &, const MapObjectData &)> m_onPropertyChange;
    std::function<void(const std::string &)> m_onSkyboxSelected;
    bool m_isVisible = true;
};
} // namespace CHEngine
