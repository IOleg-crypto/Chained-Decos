#ifndef CH_CAMERA_COMPONENT_H
#define CH_CAMERA_COMPONENT_H

#include "raylib.h"
#include "../reflect.h"

namespace CHEngine
{
struct CameraComponent
{
    float Fov = 60.0f;
    Vector3 Offset = {0.0f, 2.0f, -5.0f};
    Vector2 MousePosition = {0.0f, 0.0f};
    
    // New: Camera control flags
    bool IsActive = true;
    bool IsPrimary = false;  // If true, this is the main game camera
    float NearPlane = 0.1f;
    float FarPlane = 1000.0f;
    int Projection = 0;  // 0 = Perspective, 1 = Orthographic

    CameraComponent() = default;
};

BEGIN_REFLECT(CameraComponent)
    PROPERTY(bool, IsPrimary, "Primary")
    PROPERTY(bool, IsActive, "Active")
    PROPERTY(float, Fov, "FOV")
    PROPERTY(Vector3, Offset, "Offset")
    PROPERTY(float, NearPlane, "Near Plane")
    PROPERTY(float, FarPlane, "Far Plane")
    PROPERTY(int, Projection, "Projection")
END_REFLECT()
} // namespace CHEngine

#endif // CH_CAMERA_COMPONENT_H
