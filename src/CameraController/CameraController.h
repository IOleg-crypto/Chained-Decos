//
// Created by I#Oleg.
//

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <raylib.h>
#include <raymath.h>

class CameraController
{
  private:
    Camera m_camera;  // Raylib camera struct to represent 3D perspective
    int m_cameraMode; // Mode for camera(First , Free , Third , orbital)
    float m_baseCameraY = 4.5f;
    float m_cameraYaw = 0.0f;
    float m_cameraPitch = 0.0f;
    float m_cameraSmoothingFactor = 4.0f; // Camera smoothing speed
  public:
    CameraController(); // Init camera
    ~CameraController() = default;
    CameraController(const CameraController &other) = delete;
    CameraController(CameraController &&other) = delete;

  public:
    [[nodiscard]] Camera &GetCamera(); // Returns the current camera state (read-only)
    [[nodiscard]] int &GetCameraMode();
    void SetCameraMode(int cameraMode);
    void Update();
    void UpdateCameraRotation(); // Update camera rotation based on mouse input
    // Camera connects with jump
    [[deprecated("Used before , when player is not cube")]] void
    ApplyJumpToCamera(Camera &camera, const Vector3 &baseTarget, float jumpOffsetY);

    // Getters for camera yaw and pitch
    [[nodiscard]] float GetCameraYaw() const;
    [[nodiscard]] float GetCameraPitch() const;
    [[nodiscard]] float GetCameraSmoothingFactor() const;
};

#endif // CAMERACONTROLLER_H
