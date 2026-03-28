#ifndef CH_CAMERA_TYPES_H
#define CH_CAMERA_TYPES_H

#include <glm/glm.hpp>

namespace CHEngine
{
struct Camera2D
{
    glm::vec2 Offset = {0.0f, 0.0f};
    glm::vec2 Target = {0.0f, 0.0f};
    float Rotation = 0.0f;
    float Zoom = 1.0f;
};

struct Camera3D
{
    Camera3D() = default;

    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::vec3 Target = {0.0f, 0.0f, 0.0f};
    glm::vec3 Up = {0.0f, 1.0f, 0.0f};
    float Fovy = 45.0f;
    int Projection = 0; // 0 for Perspective, 1 for Orthographic
};
}

#endif // CH_CAMERA_TYPES_H
