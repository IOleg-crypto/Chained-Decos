#ifndef ICAMERAMANAGER_H
#define ICAMERAMANAGER_H

#include <memory>
#include <raylib.h>
#include "scene/3d/camera/Core/CameraController.h"

class ICameraManager {
public:
    virtual ~ICameraManager() = default;
    virtual void Update() = 0;
    virtual Camera3D& GetCamera() = 0;
    virtual std::shared_ptr<CameraController> GetController() = 0;
};

#endif // ICAMERAMANAGER_H