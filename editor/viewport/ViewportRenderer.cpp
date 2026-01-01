#include "ViewportRenderer.h"
#include "editor/utils/EditorGrid.h"
#include "scene/resources/map/MapRenderer.h"
#include "scene/resources/texture/TextureService.h"
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

void ViewportRenderer::RenderUIBackground(const MapMetadata &meta, uint32_t width, uint32_t height)
{
    // 1. Draw solid color if alpha > 0
    if (meta.backgroundColor.a > 0)
    {
        DrawRectangle(0, 0, (int)width, (int)height, meta.backgroundColor);
    }

    // 2. Draw texture if path provided
    if (!meta.backgroundTexture.empty())
    {
        Texture2D tex = TextureService::GetTexture(meta.backgroundTexture);
        if (tex.id == 0)
        {
            TextureService::LoadTexture(meta.backgroundTexture, meta.backgroundTexture);
            tex = TextureService::GetTexture(meta.backgroundTexture);
        }

        if (tex.id != 0)
        {
            Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
            Rectangle dest = {0, 0, (float)width, (float)height};
            DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, WHITE);
        }
    }
}

void ViewportRenderer::RenderUIElements(const std::vector<UIElementData> &elements,
                                        int selectedIndex)
{
    for (int i = 0; i < (int)elements.size(); i++)
    {
        const auto &el = elements[i];
        bool isSelected = (i == selectedIndex);

        if (el.type == "button")
        {
            DrawRectangle((int)el.position.x, (int)el.position.y, (int)el.size.x, (int)el.size.y,
                          el.normalColor);
            DrawText(el.text.c_str(), (int)el.position.x + 5, (int)el.position.y + 5, 20, BLACK);
        }
        else if (el.type == "text")
        {
            DrawText(el.text.c_str(), (int)el.position.x, (int)el.position.y, el.fontSize,
                     el.textColor);
        }

        if (isSelected)
        {
            DrawRectangleLines((int)el.position.x - 2, (int)el.position.y - 2, (int)el.size.x + 4,
                               (int)el.size.y + 4, YELLOW);
        }
    }
}
} // namespace CHEngine
