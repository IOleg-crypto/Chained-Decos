//
// Created by I#Oleg
//

#include "Editor.h"

Editor::Editor() : m_cameraController(std::make_unique<CameraController>())
{

}

Editor::~Editor() = default;

std::shared_ptr<CameraController> Editor::GetCameraController() const {
     return m_cameraController;
}
