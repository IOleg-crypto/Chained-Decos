#include "CameraManager.h"
#include "Engine/CameraController/CameraController.h"
#include <imgui.h>
#include <raylib.h>
#include <raymath.h>

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
        if (io.WantCaptureMouse || io.WantCaptureKeyboard)
        {
            return;
        }

        Camera& camera = m_cameraController->GetCamera();
        bool leftMousePressed = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        bool arrowKeysPressed = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN);
        
        // Handle rotation: only when left mouse button is pressed or arrow keys are used
        if (leftMousePressed)
        {
            // Update rotation based on mouse delta when left mouse button is pressed
            m_cameraController->UpdateCameraRotation();
        }
        else if (arrowKeysPressed)
        {
            // Update rotation based on arrow keys
            float rotationSpeed = 90.0f; // degrees per second
            float deltaTime = GetFrameTime();
            
            // Calculate camera forward and right vectors for rotation
            Vector3 forward = Vector3Subtract(camera.target, camera.position);
            forward = Vector3Normalize(forward);
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
            
            // Rotate around up vector (yaw) with left/right arrows
            if (IsKeyDown(KEY_LEFT))
            {
                // Rotate target left around camera position
                Matrix rotation = MatrixRotateY(DEG2RAD * rotationSpeed * deltaTime);
                forward = Vector3Transform(forward, rotation);
                camera.target = Vector3Add(camera.position, forward);
            }
            if (IsKeyDown(KEY_RIGHT))
            {
                // Rotate target right around camera position
                Matrix rotation = MatrixRotateY(DEG2RAD * -rotationSpeed * deltaTime);
                forward = Vector3Transform(forward, rotation);
                camera.target = Vector3Add(camera.position, forward);
            }
            
            // Rotate around right vector (pitch) with up/down arrows
            if (IsKeyDown(KEY_UP))
            {
                // Rotate target up
                Matrix rotation = MatrixRotate(right, DEG2RAD * rotationSpeed * deltaTime);
                forward = Vector3Transform(forward, rotation);
                camera.target = Vector3Add(camera.position, forward);
            }
            if (IsKeyDown(KEY_DOWN))
            {
                // Rotate target down
                Matrix rotation = MatrixRotate(right, DEG2RAD * -rotationSpeed * deltaTime);
                forward = Vector3Transform(forward, rotation);
                camera.target = Vector3Add(camera.position, forward);
            }
        }
        
        // Always handle camera movement (WASD) separately from rotation
        // This allows movement without unwanted rotation
        // Save current camera mode to restore it after movement
        int savedMode = m_cameraController->GetCameraMode();
        m_cameraController->SetCameraMode(CAMERA_FREE);
        UpdateCamera(&camera, CAMERA_FREE);
        m_cameraController->SetCameraMode(savedMode);
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