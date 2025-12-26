#pragma once

#include "scene/resources/map/GameScene.h"
#include <functional>
#include <imgui.h>
#include <raylib.h>

namespace CHEngine
{

/**
 * @brief Hazel-style viewport picking system
 * Handles ray casting and object selection in the editor viewport
 */
class ViewportPicking
{
public:
    ViewportPicking() = default;
    ~ViewportPicking() = default;

    /**
     * @brief Perform picking operation to select objects
     * @param mousePos Current mouse position in screen space
     * @param viewportPos Viewport panel position
     * @param viewportSize Viewport panel size
     * @param camera Current camera
     * @param scene Scene containing objects
     * @return Index of selected object, or -1 if none
     */
    int PickObject(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize, const Camera3D &camera,
                   const std::shared_ptr<GameScene> &scene);

    /**
     * @brief Get ray from mouse position (Hazel-style)
     * Properly converts screen coordinates to world ray with NDC
     */
    Ray GetMouseRay(ImVec2 mousePos, ImVec2 viewportPos, ImVec2 viewportSize,
                    const Camera3D &camera);

private:
    /**
     * @brief Test ray collision with a map object
     */
    RayCollision TestObjectCollision(const Ray &ray, const MapObjectData &obj);
};

} // namespace CHEngine
