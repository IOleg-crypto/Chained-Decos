//
// Created by I#Oleg.
//

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <raylib.h>

class CameraController {
private:
    Camera m_camera; // Raylib camera struct to represent 3D perspective
    int m_cameraMode; // Mode for camera(First , Free , Third , orbital)
public:
    CameraController(); // Init camera
    ~CameraController() = default;
    CameraController(const CameraController&other) = delete;
    CameraController(CameraController &&other) = delete;
public:
    [[nodiscard]] Camera &GetCamera(); // Returns the current camera state (read-only)
    [[nodiscard]] int &GetCameraMode();
    void SetCameraMode(int cameraMode);
    void Update();
};



#endif //CAMERACONTROLLER_H
