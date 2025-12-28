#pragma once

#include "scene/resources/map/GameScene.h"
#include <functional>
#include <imgui.h>
#include <raylib.h>

namespace CHEngine
{

class ViewportPicking
{
public:
    ViewportPicking() = default;
    ~ViewportPicking() = default;

    int PickObject(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize, const Camera3D &camera,
                   const std::shared_ptr<GameScene> &scene);
    Ray GetMouseRay(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                    const Camera3D &camera);

private:
    RayCollision TestObjectCollision(const Ray &ray, const MapObjectData &obj);
};

} // namespace CHEngine
