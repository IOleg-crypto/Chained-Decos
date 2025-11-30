#ifndef CAMERA_COMPONENT_H
#define CAMERA_COMPONENT_H

#include <raylib.h>

struct CameraComponent
{
    Camera camera;

    // Camera settings
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    // Camera control
    bool isActive = false; // Тільки одна камера може бути активною
    int priority = 0;      // Вища пріоритетність = активна камера

    // Camera shake
    bool isShaking = false;
    float shakeIntensity = 0.0f;
    float shakeDuration = 0.0f;
    float shakeTimer = 0.0f;
};

#endif // CAMERA_COMPONENT_H
