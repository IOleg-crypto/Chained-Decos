#include "viewport_renderer.h"
#include "editor/utils/editor_grid.h"
#include "engine/scene/core/scene.h"
#include "engine/scene/ecs/components/render_component.h"
#include "engine/scene/ecs/components/transform_component.h"
#include <raymath.h>

namespace CHEngine
{
void ViewportRenderer::RenderSelectionHighlight(entt::entity entity, Scene &scene,
                                                const Camera3D &camera)
{
    if (entity == entt::null)
        return;

    auto &registry = scene.GetRegistry();
    if (!registry.all_of<TransformComponent, RenderComponent>(entity))
        return;

    auto &transform = registry.get<TransformComponent>(entity);
    auto &render = registry.get<RenderComponent>(entity);

    if (!render.model)
        return;

    // Draw selection highlight (wireframe or colored model)
    // Simplified for now: just draw the model with a tint or wireframe
    Matrix translation = MatrixTranslate(transform.position.x + render.offset.x,
                                         transform.position.y + render.offset.y,
                                         transform.position.z + render.offset.z);
    Matrix rotation = MatrixRotateXYZ(transform.rotation);
    Matrix scale = MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);

    Matrix modelTransform = MatrixMultiply(MatrixMultiply(scale, rotation), translation);
    Matrix originalTransform = render.model->transform;
    render.model->transform = modelTransform;

    DrawModelWires(*render.model, {0, 0, 0}, 1.01f, YELLOW);

    render.model->transform = originalTransform;
}

void ViewportRenderer::RenderAxisLabels(entt::entity entity, Scene &scene, const Camera3D &camera,
                                        int currentTool, float gizmoSize)
{
    if (entity == entt::null)
        return;

    auto &registry = scene.GetRegistry();
    if (!registry.all_of<TransformComponent>(entity))
        return;

    auto &transform = registry.get<TransformComponent>(entity);

    auto drawLabel = [&](Vector3 dir, const char *label, Color color)
    {
        Vector3 endPos = Vector3Add(transform.position, Vector3Scale(dir, gizmoSize));
        Vector2 screenPos = GetWorldToScreen(endPos, camera);

        DrawText(label, (int)screenPos.x + 5, (int)screenPos.y - 10, 20, color);
    };

    drawLabel({1, 0, 0}, "X", RED);
    drawLabel({0, 1, 0}, "Y", GREEN);
    drawLabel({0, 0, 1}, "Z", BLUE);
}

void ViewportRenderer::RenderGrid(const Camera3D &camera, uint32_t width, uint32_t height)
{
    // Grid logic is handled by EditorGrid class in ViewportPanel for now
}

} // namespace CHEngine
