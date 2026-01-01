#ifndef VIEWPORT_RENDERER_H
#define VIEWPORT_RENDERER_H

#include "editor/EditorTypes.h"
#include "scene/resources/map/GameScene.h"
#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{
class ViewportRenderer
{
public:
    ViewportRenderer() = default;

    void RenderSelectionHighlight(const MapObjectData &obj,
                                  const std::unordered_map<std::string, Model> &models,
                                  const Camera3D &camera);

    void RenderAxisLabels(const MapObjectData &obj, const Camera3D &camera, Tool currentTool,
                          float gizmoSize = 2.0f);

    void RenderGrid(const Camera3D &camera, uint32_t width, uint32_t height);
    void RenderUIBackground(const MapMetadata &meta, uint32_t width, uint32_t height);
    void RenderUIElements(const std::vector<UIElementData> &elements, int selectedIndex);
};
} // namespace CHEngine

#endif // VIEWPORT_RENDERER_H
