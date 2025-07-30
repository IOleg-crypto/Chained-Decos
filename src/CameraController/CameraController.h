//
// Created by I#Oleg.
//

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include <raylib.h>

class CameraController {
private:
    Camera camera; // Raylib camera struct to represent 3D perspective
    int cameraMode; // Mode for camera(First , Free , Third , orbital)
public:
    CameraController(); // Init camera
    [[nodiscard]] Camera &getCamera(); // Returns the current camera state (read-only)
    [[nodiscard]] int &GetCameraMode();
    void SetCameraMode(const int cameraMode);
    void Update();
};



#endif //CAMERACONTROLLER_H
