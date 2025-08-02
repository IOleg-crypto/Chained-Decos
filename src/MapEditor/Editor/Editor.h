//
// Created by I#Oleg.
//

#ifndef EDITOR_H
#define EDITOR_H

#include <imgui.h>
#include <raylib.h>
#include <memory>

#include <CameraController/CameraController.h>


class Editor {
private:
     std::shared_ptr<CameraController>m_cameraController;
public:
     Editor();
     ~Editor();
public:
     [[nodiscard]] std::shared_ptr<CameraController>GetCameraController() const;
     
};



#endif //EDITOR_H
