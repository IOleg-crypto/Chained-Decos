#pragma once

#include "scene/resources/map/core/MapData.h"
#include <imgui.h>
#include <string>

namespace CHEngine
{
class InspectorPanel
{
public:
    InspectorPanel() = default;

    void OnImGuiRender(MapObjectData *selectedEntity);

private:
    void DrawComponents(MapObjectData *entity);
    void DrawVec3Control(const std::string &label, Vector3 &values, float resetValue = 0.0f,
                         float columnWidth = 100.0f);
};
} // namespace CHEngine
