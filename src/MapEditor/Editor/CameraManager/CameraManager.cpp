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
            // Update rotation angles based on mouse delta when left mouse button is pressed
            m_cameraController->UpdateCameraRotation();
            
            // Apply rotation to camera manually using Vector3 and Matrix operations
            Vector2 mouseDelta = GetMouseDelta();
            float sensitivity = 0.005f;
            
            // Calculate forward vector from camera position to target
            Vector3 forward = Vector3Subtract(camera.target, camera.position);
            float distance = Vector3Length(forward);
            forward = Vector3Normalize(forward);
            
            // Get right and up vectors
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
            Vector3 upVec = Vector3Normalize(Vector3CrossProduct(right, forward));
            
            // Apply yaw rotation (horizontal) - rotate around camera up vector
            float yawDelta = -mouseDelta.x * sensitivity;
            if (yawDelta != 0.0f)
            {
                Matrix yawRotation = MatrixRotate(camera.up, yawDelta);
                forward = Vector3Transform(forward, yawRotation);
                forward = Vector3Normalize(forward);
                // Recalculate right vector after yaw rotation
                right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
            }
            
            // Apply pitch rotation (vertical) - rotate around right vector
            float pitchDelta = -mouseDelta.y * sensitivity;
            if (pitchDelta != 0.0f)
            {
                // Clamp pitch to avoid gimbal lock
                Vector3 forwardBefore = forward;
                Matrix pitchRotation = MatrixRotate(right, pitchDelta);
                forward = Vector3Transform(forward, pitchRotation);
                forward = Vector3Normalize(forward);
                
                // Check if we've rotated too far up/down (prevent flipping)
                float dotUp = Vector3DotProduct(forward, camera.up);
                if (dotUp > 0.99f || dotUp < -0.99f)
                {
                    forward = forwardBefore;
                }
            }
            
            // Update camera target based on new forward direction
            camera.target = Vector3Add(camera.position, Vector3Scale(forward, distance));
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
        
        // Handle camera movement (WASD) manually - don't use UpdateCamera which auto-rotates
        // Only allow rotation when left mouse button is pressed
        // This prevents unwanted rotation when clicking objects
        
        // Get camera movement speed with acceleration support
        float deltaTime = GetFrameTime();
        float baseMoveSpeed = 5.0f; // Base speed in units per second
        float speedMultiplier = 1.0f;
        
        // Acceleration: Hold Shift for faster movement
        if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
        {
            speedMultiplier = 3.0f; // 3x faster when holding Shift
        }
        
        float moveSpeed = baseMoveSpeed * speedMultiplier;
        float moveDistance = moveSpeed * deltaTime;
        
        // Camera movement with keyboard (WASD) - manual implementation
        // Calculate camera direction vectors
        Vector3 forward = Vector3Subtract(camera.target, camera.position);
        forward = Vector3Normalize(forward);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
        
        // For free camera, move on world plane (ignore Y component for horizontal movement)
        Vector3 forwardPlane = forward;
        forwardPlane.y = 0.0f;
        forwardPlane = Vector3Normalize(forwardPlane);
        
        Vector3 rightPlane = right;
        rightPlane.y = 0.0f;
        rightPlane = Vector3Normalize(rightPlane);
        
        // Move forward/backward (W/S)
        if (IsKeyDown(KEY_W))
        {
            Vector3 moveDir = Vector3Scale(forwardPlane, moveDistance);
            camera.position = Vector3Add(camera.position, moveDir);
            camera.target = Vector3Add(camera.target, moveDir);
        }
        if (IsKeyDown(KEY_S))
        {
            Vector3 moveDir = Vector3Scale(forwardPlane, -moveDistance);
            camera.position = Vector3Add(camera.position, moveDir);
            camera.target = Vector3Add(camera.target, moveDir);
        }
        
        // Move left/right (A/D)
        if (IsKeyDown(KEY_A))
        {
            Vector3 moveDir = Vector3Scale(rightPlane, -moveDistance);
            camera.position = Vector3Add(camera.position, moveDir);
            camera.target = Vector3Add(camera.target, moveDir);
        }
        if (IsKeyDown(KEY_D))
        {
            Vector3 moveDir = Vector3Scale(rightPlane, moveDistance);
            camera.position = Vector3Add(camera.position, moveDir);
            camera.target = Vector3Add(camera.target, moveDir);
        }
        
        // Vertical movement with Space/Ctrl (Shift is now used for acceleration)
        Vector3 worldUp = {0.0f, 1.0f, 0.0f};
        if (IsKeyDown(KEY_SPACE))
        {
            Vector3 moveDir = Vector3Scale(worldUp, moveDistance);
            camera.position = Vector3Add(camera.position, moveDir);
            camera.target = Vector3Add(camera.target, moveDir);
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        {
            Vector3 moveDir = Vector3Scale(worldUp, -moveDistance);
            camera.position = Vector3Add(camera.position, moveDir);
            camera.target = Vector3Add(camera.target, moveDir);
        }
        
        // Zoom with mouse wheel - move camera closer/farther to target
        float wheelMove = GetMouseWheelMove();
        if (wheelMove != 0.0f)
        {
            float zoomDistance = -wheelMove * 2.0f;
            Vector3 zoomDirection = Vector3Normalize(forward);
            Vector3 zoomOffset = Vector3Scale(zoomDirection, zoomDistance);
            camera.position = Vector3Add(camera.position, zoomOffset);
        }
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