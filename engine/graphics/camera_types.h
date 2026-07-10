#ifndef CH_CAMERA_TYPES_H
#define CH_CAMERA_TYPES_H

#include <glm/glm.hpp>

namespace Chained
{

enum class ProjectionType : int
{
    Perspective = 0,
    Orthographic = 1
};

struct Camera2D
{
    glm::vec2 Position = {0.0f, 0.0f};
    float Rotation = 0.0f;
    float Zoom = 1.0f;
    float NearClip = -1.0f;
    float FarClip = 1.0f;

    glm::mat4 ViewMatrix = glm::mat4(1.0f);
    glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
};

struct Camera3D
{
    Camera3D() = default;

    glm::vec3 Position = {0.0f, 0.0f, 0.0f};
    glm::vec3 Target = {0.0f, 0.0f, 0.0f};
    glm::vec3 Up = {0.0f, 1.0f, 0.0f};

    ProjectionType Projection = ProjectionType::Perspective;
    float FovDegrees = 45.0f;
    float OrthographicSize = 10.0f;
    float NearClip = 0.01f;
    float FarClip = 1000.0f;

    glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
    glm::mat4 ViewMatrix = glm::mat4(1.0f);
};

} // namespace Chained

#endif // CH_CAMERA_TYPES_H
