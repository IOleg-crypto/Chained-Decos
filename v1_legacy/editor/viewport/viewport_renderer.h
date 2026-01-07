#ifndef CD_EDITOR_VIEWPORT_VIEWPORT_RENDERER_H
#define CD_EDITOR_VIEWPORT_VIEWPORT_RENDERER_H

#include <entt/entt.hpp>
#include <raylib.h>
#include <string>

namespace CHEngine
{
class Scene;

class ViewportRenderer
{
public:
    ViewportRenderer() = default;

public:
    void RenderSelectionHighlight(entt::entity entity, Scene &scene, const Camera3D &camera);
    void RenderAxisLabels(entt::entity entity, Scene &scene, const Camera3D &camera,
                          int currentTool, float gizmoSize = 2.0f);

    void RenderGrid(const Camera3D &camera, uint32_t width, uint32_t height);
};
} // namespace CHEngine

#endif // CD_EDITOR_VIEWPORT_VIEWPORT_RENDERER_H
