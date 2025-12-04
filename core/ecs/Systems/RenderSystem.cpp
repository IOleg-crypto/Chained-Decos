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

    // Сортувати по renderLayer для правильного порядку відображення
    auto view = REGISTRY.view<TransformComponent, RenderComponent>();

    std::vector<entt::entity> entities(view.begin(), view.end());
    std::sort(entities.begin(), entities.end(),
              [&](entt::entity a, entt::entity b)
              {
                  auto &renderA = view.get<RenderComponent>(a);
                  auto &renderB = view.get<RenderComponent>(b);
                  return renderA.renderLayer < renderB.renderLayer;
              });

    // Рендерити
    for (auto entity : entities)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &renderComp = view.get<RenderComponent>(entity);

        if (!renderComp.visible || !renderComp.model)
        {
            continue;
        }

        // Застосувати transform до моделі
        Matrix matScale = MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);
        Matrix matRotation = MatrixRotateXYZ(transform.rotation);
        Matrix matTranslation =
            MatrixTranslate(transform.position.x, transform.position.y, transform.position.z);
        renderComp.model->transform =
            MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

        // Рендерити модель
        DrawModel(*renderComp.model, transform.position,
                  1.0f, // scale (вже в transform)
                  renderComp.tint);
    }
}

} // namespace RenderSystem
