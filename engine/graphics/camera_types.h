#ifndef CH_CAMERA_TYPES_H
#define CH_CAMERA_TYPES_H

#include <glm/glm.hpp>

// Not working , but in the future , i guess i do 2D Renderer
namespace Chained
{
struct Camera2D
{
    glm::vec2 Offset = {0.0f, 0.0f};
    glm::vec2 Target = {0.0f, 0.0f};
    float Rotation = 0.0f;
    float Zoom = 1.0f;
};
// Goat camera
struct Camera3D
{
    Camera3D() = default;

    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::vec3 Target = {0.0f, 0.0f, 0.0f};
    glm::vec3 Up = {0.0f, 1.0f, 0.0f};
    float FovY = 45.0f;
    float NearClip = 0.01f;
    float FarClip = 1000.0f;
    int Projection = 0; // 0 for Perspective, 1 for Orthographic
    
    // Explicit matrices to avoid double calculation in Renderers
    glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
    glm::mat4 ViewMatrix = glm::mat4(1.0f);
};
} // namespace Chained

#endif // CH_CAMERA_TYPES_H
