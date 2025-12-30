#include "ViewportRenderer.h"
#include "editor/utils/EditorGrid.h"
#include "scene/resources/map/MapRenderer.h"
#include <raymath.h>

namespace CHEngine
{
void ViewportRenderer::RenderSelectionHighlight(
    const MapObjectData &obj, const std::unordered_map<std::string, Model> &models,
    const Camera3D &camera)
{
    MapRenderer renderer;
    std::unordered_map<std::string, Texture2D> emptyTextures;
    renderer.RenderMapObject(obj, models, emptyTextures, camera, true, true);
}

void ViewportRenderer::RenderAxisLabels(const MapObjectData &obj, const Camera3D &camera,
                                        Tool currentTool, float gizmoSize)
{
    if (currentTool == Tool::SELECT)
        return;

    auto drawLabel = [&](Vector3 dir, const char *label, Color color)
    {
        Vector3 endPos = Vector3Add(obj.position, Vector3Scale(dir, gizmoSize));
        Vector2 screenPos = GetWorldToScreen(endPos, camera);

        DrawText(label, (int)screenPos.x + 5, (int)screenPos.y - 10, 20, color);
    };

    drawLabel({1, 0, 0}, "X", RED);
    drawLabel({0, 1, 0}, "Y", GREEN);
    drawLabel({0, 0, 1}, "Z", BLUE);
}

void ViewportRenderer::RenderGrid(const Camera3D &camera, uint32_t width, uint32_t height)
{
    // Grid logic can be centralized here if needed
}
} // namespace CHEngine
