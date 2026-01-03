#ifndef CD_EDITOR_VIEWPORT_VIEWPORT_PICKING_H
#define CD_EDITOR_VIEWPORT_VIEWPORT_PICKING_H

#include <entt/entt.hpp>
#include <imgui.h>
#include <raylib.h>

namespace CHEngine
{
class Scene;

class ViewportPicking
{
public:
    ViewportPicking() = default;
    ~ViewportPicking() = default;

    entt::entity PickEntity(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                            const Camera3D &camera, Scene &scene);
    Ray GetMouseRay(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                    const Camera3D &camera);

private:
};

} // namespace CHEngine

#endif // CD_EDITOR_VIEWPORT_VIEWPORT_PICKING_H
