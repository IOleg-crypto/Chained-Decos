//
// Created by I#Oleg.
//

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <raylib.h>
#include <raymath.h>
#include "ICameraSensitivityController.h"

//
// CameraController
// Handles the 3D camera system including position, rotation, mode, and input smoothing.
// Supports different camera modes (Third) and mouse-based rotation.
//
class CameraController : public ICameraSensitivityController
{
public:
    CameraController();  // Initialize camera
    ~CameraController() = default;

    CameraController(const CameraController &other) = delete;
    CameraController(CameraController &&other) = delete;

    // -------------------- Accessors --------------------

    // Get current camera reference (modifiable)
    [[nodiscard]] Camera &GetCamera();

    // Get reference to camera mode integer
    [[nodiscard]] int &GetCameraMode();

    // Set camera mode
    void SetCameraMode(int cameraMode);

    // -------------------- Updates --------------------

    // Update camera logic (called every frame)
    void Update();

    // Update camera rotation based on mouse input
    void UpdateCameraRotation();

    // Update camera offset based on FOV and angles relative to player position
    void UpdateMouseRotation(Camera &camera, const Vector3 &playerPosition);

    // -------------------- Settings --------------------

    // Set field of view (radius)
    void SetFOV(float FOV); 
    void SetMouseSensitivity(float sensitivity) override;
    void ApplyJumpToCamera(Camera &camera, const Vector3 &baseTarget, float jumpOffsetY);

    // -------------------- Getters --------------------

    [[nodiscard]] float GetCameraYaw() const;
    [[nodiscard]] float GetCameraPitch() const;
    [[nodiscard]] float GetCameraSmoothingFactor() const;
    [[nodiscard]] float GetFOV() const;
    [[nodiscard]] float GetMouseSensitivity() const override;

    // -------------------- Static Utilities --------------------
    
    // Static utility function for filtering mouse delta (prevents glitches on Linux/VM)
    // Use this everywhere GetMouseDelta() is called to ensure consistent behavior
    static Vector2 FilterMouseDelta(const Vector2& mouseDelta);

private:
    Camera m_camera;                  // Raylib camera struct representing the 3D perspective
    int m_cameraMode;                 // Current camera mode (First, Free, Third, Orbital)
    float m_baseCameraY = 4.5f;      // Base camera height offset
    float m_cameraYaw = 1.0f;        // Yaw angle for rotation
    float m_cameraPitch = 0.0f;      // Pitch angle for rotation
    float m_cameraSmoothingFactor = 4.0f; // Smoothing speed for camera rotation
    float m_radiusFOV = 8.0f;        // Radius or distance for field of view

    float m_mouseSensitivity = 0.1f; // Mouse sensitivity
    // Smoothing for virtual machines
    Vector2 m_smoothedMouseDelta = {0.0f, 0.0f}; // Smoothed mouse delta value
    static constexpr float MOUSE_DEAD_ZONE = 0.5f; // Dead zone - ignore very small movements
};

#endif // CAMERACONTROLLER_H
