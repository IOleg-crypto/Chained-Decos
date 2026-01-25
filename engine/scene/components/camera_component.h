#ifndef CH_CAMERA_COMPONENT_H
#define CH_CAMERA_COMPONENT_H

#include <raylib.h>

namespace CHEngine
{
struct CameraComponent
{
    float Fov = 110.0f;
    Vector3 Offset = {0.0f, 2.0f, -5.0f}; // Offset from target
    Vector2 MousePosition = {0.0f, 0.0f};

    CameraComponent() = default;
};
} // namespace CHEngine

#endif // CH_CAMERA_COMPONENT_H
