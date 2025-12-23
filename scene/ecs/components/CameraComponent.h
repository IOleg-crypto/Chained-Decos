#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <raylib.h>

namespace CHEngine
{
struct CameraComponent
{
    ::Camera camera; // Explicit Raylib Camera

    // Camera settings
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    // Camera control
    bool isActive = false;
    int priority = 0;
    // Camera shake
    bool isShaking = false;
    float shakeIntensity = 0.0f;
    float shakeDuration = 0.0f;
    float shakeTimer = 0.0f;
};
} // namespace CHEngine

#endif // CAMERA_COMPONENT_H
