#include "render_system.h"
#include "engine/scene/ecs/components/render_component.h"
#include "engine/scene/ecs/components/transform_component.h"
#include <algorithm>
#include <raymath.h>
#include <vector>


namespace CHEngine
{

void RenderSystem::Render(entt::registry &registry)
{
    auto view = registry.view<TransformComponent, RenderComponent>();

    // Sort entities by render layer to ensure correct draw order
    std::vector<entt::entity> sortedEntities(view.begin(), view.end());
    std::sort(sortedEntities.begin(), sortedEntities.end(),
              [&](entt::entity a, entt::entity b)
              {
                  return view.get<RenderComponent>(a).renderLayer <
                         view.get<RenderComponent>(b).renderLayer;
              });

    for (auto entity : sortedEntities)
    {
        auto &transform = view.get<TransformComponent>(entity);
        auto &render = view.get<RenderComponent>(entity);

        if (!render.visible || !render.model)
            continue;

        // Calculate model transformation matrix
        Matrix matS = MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);
        Matrix matR = MatrixRotateXYZ(transform.rotation);
        Matrix matT = MatrixTranslate(transform.position.x + render.offset.x,
                                      transform.position.y + render.offset.y,
                                      transform.position.z + render.offset.z);

        render.model->transform = MatrixMultiply(MatrixMultiply(matS, matR), matT);

        // Handle texture override if present
        Texture2D originalTexture;
        bool hasTextureOverride = (render.texture != nullptr);
        if (hasTextureOverride)
        {
            originalTexture = render.model->materials[0].maps[MATERIAL_MAP_ALBEDO].texture;
            render.model->materials[0].maps[MATERIAL_MAP_ALBEDO].texture = *render.texture;
        }

        // Render the model
        DrawModel(*render.model, Vector3Zero(), 1.0f, render.tint);

        // Restore original texture if it was overridden
        if (hasTextureOverride)
        {
            render.model->materials[0].maps[MATERIAL_MAP_ALBEDO].texture = originalTexture;
        }
    }
}

} // namespace CHEngine
