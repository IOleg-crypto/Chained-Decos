#include "CameraManager.h"
#include "Engine/CameraController/CameraController.h"
#include <imgui.h>

CameraManager::CameraManager(std::shared_ptr<CameraController> cameraController)
    : m_cameraController(std::move(cameraController))
{
}

void CameraManager::Update()
{
    if (m_cameraController)
    {
        // If ImGui is capturing mouse (e.g., hovering tools window), skip camera update
        const ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }

        m_cameraController->Update();
    }
}

Camera3D& CameraManager::GetCamera()
{
    return m_cameraController->GetCamera();
}

std::shared_ptr<CameraController> CameraManager::GetController()
{
    return m_cameraController;
}