#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include "ICameraManager.h"
#include <memory>

#include "Engine/CameraController/Core/CameraController.h"

class CameraManager : public ICameraManager {
private:
    std::shared_ptr<CameraController> m_cameraController;

public:
    explicit CameraManager(std::shared_ptr<CameraController> cameraController);
    ~CameraManager() override = default;

    void Update() override;
    Camera3D& GetCamera() override;
    std::shared_ptr<CameraController> GetController() override;
};

#endif // CAMERAMANAGER_H