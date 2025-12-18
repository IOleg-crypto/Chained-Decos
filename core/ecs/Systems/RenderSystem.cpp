#include "RenderSystem.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/ecs/Components/RenderComponent.h"
#include "core/ecs/Components/TransformComponent.h"
#include "core/ecs/ECSRegistry.h"
#include <algorithm>
#include <raylib.h>
#include <raymath.h>
#include <vector>

namespace RenderSystem
{

void Render()
{
    auto &render = RenderManager::Get();

    // Sort by renderLayer for correct display order
    auto view = REGISTRY.view<TransformComponent, RenderComponent>();

    std::vector<entt::entity> entities(view.begin(), view.end());
    std::sort(entities.begin(), entities.end(),
              [&](entt::entity a, entt::entity b)
              {
                  auto &renderA = view.get<RenderComponent>(a);
                  auto &renderB = view.get<RenderComponent>(b);
                  return renderA.renderLayer < renderB.renderLayer;
              });

    // Render
    for (auto entity : entities)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &renderComp = view.get<RenderComponent>(entity);

        if (!renderComp.visible || !renderComp.model)
        {
            continue;
        }

        // Apply transform to model
        Matrix matScale = MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);
        Matrix matRotation = MatrixRotateXYZ(transform.rotation);
        // Apply offset to translation matrix so it affects shader transform too
        Vector3 finalPos = Vector3Add(transform.position, renderComp.offset);
        Matrix matTranslation = MatrixTranslate(finalPos.x, finalPos.y, finalPos.z);

        renderComp.model->transform =
            MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

        // Render model at the offset position
        // Fix: Pass Vector3Zero() because the transform matrix already includes the translation
        // (finalPos) Passing finalPos again leads to double translation
        DrawModel(*renderComp.model, Vector3Zero(),
                  1.0f, // scale (already in transform)
                  renderComp.tint);
    }
}

} // namespace RenderSystem
