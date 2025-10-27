//
// Created by I#Oleg
//

#include "CameraModes.h"
#include <cmath>

void CameraModes::UpdateFreeCamera(Camera& camera) {
    // Handle mouse look and keyboard movement for free camera
    HandleMouseLook(camera);
    HandleKeyboardMovement(camera);
}

void CameraModes::UpdateOrbitalCamera(Camera& camera, Vector3 target) {
    // Static variables to maintain camera state
    static float distance = 10.0f;    // Distance from target
    static float angleX = 0.0f;       // Vertical angle
    static float angleY = 0.0f;       // Horizontal angle
    
    // Handle mouse input for orbital movement
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        const Vector2 mouseDelta = GetMouseDelta();
        angleY += mouseDelta.x * 0.01f;  // Rotate horizontally
        angleX += mouseDelta.y * 0.01f;  // Rotate vertically
        
        // Clamp vertical angle to prevent flipping
        if (angleX > PI/2 - 0.1f) angleX = PI/2 - 0.1f;
        if (angleX < -PI/2 + 0.1f) angleX = -PI/2 + 0.1f;
    }
    
    // Handle mouse wheel for zoom
    float wheel = GetMouseWheelMove();
    distance -= wheel * 0.5f;
    if (distance < 1.0f) distance = 1.0f;      // Minimum distance
    if (distance > 50.0f) distance = 50.0f;     // Maximum distance
    
    // Calculate camera position based on spherical coordinates
    float x = target.x + distance * cos(angleY) * cos(angleX);
    float y = target.y + distance * sin(angleX);
    float z = target.z + distance * sin(angleY) * cos(angleX);
    
    camera.position = {x, y, z};
    camera.target = target;
}

void CameraModes::UpdateFirstPersonCamera(Camera& camera) {
    // Static variables for camera orientation
    static float yaw = 0.0f;      // Horizontal rotation
    static float pitch = 0.0f;    // Vertical rotation
    
    // Handle mouse input for looking around
    Vector2 mouseDelta = GetMouseDelta();
    yaw += mouseDelta.x * 0.01f;      // Rotate left/right
    pitch -= mouseDelta.y * 0.01f;    // Look up/down
    
    // Clamp pitch to prevent over-rotation
    if (pitch > PI/2 - 0.1f) pitch = PI/2 - 0.1f;
    if (pitch < -PI/2 + 0.1f) pitch = -PI/2 + 0.1f;
    
    // Calculate view direction vector
    Vector3 direction;
    direction.x = cos(yaw) * cos(pitch);
    direction.y = sin(pitch);
    direction.z = sin(yaw) * cos(pitch);
    
    camera.target = Vector3Add(camera.position, direction);
    
    // Handle keyboard movement
    float speed = 0.1f;
    if (IsKeyDown(KEY_W)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(direction, speed));
    }
    if (IsKeyDown(KEY_S)) {
        camera.position = Vector3Subtract(camera.position, Vector3Scale(direction, speed));
    }
    if (IsKeyDown(KEY_A)) {
        Vector3 right = Vector3CrossProduct(direction, {0, 1, 0});
        camera.position = Vector3Subtract(camera.position, Vector3Scale(right, speed));
    }
    if (IsKeyDown(KEY_D)) {
        Vector3 right = Vector3CrossProduct(direction, {0, 1, 0});
        camera.position = Vector3Add(camera.position, Vector3Scale(right, speed));
    }
}

void CameraModes::UpdateTopDownCamera(Camera& camera) {
    // Static variables for camera state
    static float height = 20.0f;       // Camera height from ground
    static Vector3 target = {0, 0, 0}; // Camera target point
    
    // Handle mouse input for camera panning
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 mouseDelta = GetMouseDelta();
        target.x -= mouseDelta.x * 0.01f;  // Pan left/right
        target.z -= mouseDelta.y * 0.01f;  // Pan forward/back
    }
    
    // Handle mouse wheel for zoom
    float wheel = GetMouseWheelMove();
    height -= wheel * 0.5f;
    if (height < 5.0f) height = 5.0f;        // Minimum height
    if (height > 100.0f) height = 100.0f;    // Maximum height
    
    // Set camera position and target
    camera.position = {target.x, height, target.z};
    camera.target = target;
}

std::string CameraModes::GetModeName(EditorCameraMode mode) {
    // Return human-readable name for each camera mode
    switch (mode) {
        case FREE: return "Free Camera";
        case ORBITAL: return "Orbital Camera";
        case FIRST_PERSON: return "First Person";
        case TOP_DOWN: return "Top Down";
        default: return "Unknown";
    }
}

void CameraModes::SetCameraMode(Camera& camera, const EditorCameraMode mode) {
    // Initialize camera settings for each mode
    switch (mode) {
        case FREE:
            // Free camera: positioned above and looking at origin
            camera.position = {10.0f, 10.0f, 10.0f};
            camera.target = {0.0f, 0.0f, 0.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 45.0f;
            camera.projection = CAMERA_PERSPECTIVE;
            break;
            
        case ORBITAL:
            // Orbital camera: same initial position as free
            camera.position = {10.0f, 10.0f, 10.0f};
            camera.target = {0.0f, 0.0f, 0.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 45.0f;
            camera.projection = CAMERA_PERSPECTIVE;
            break;
            
        case FIRST_PERSON:
            // First-person camera: at eye level
            camera.position = {0.0f, 2.0f, 0.0f};
            camera.target = {0.0f, 2.0f, -1.0f};
            camera.up = {0.0f, 1.0f, 0.0f};
            camera.fovy = 60.0f;
            camera.projection = CAMERA_PERSPECTIVE;
            break;
            
        case TOP_DOWN:
            // Top-down camera: looking straight down
            camera.position = {0.0f, 20.0f, 0.0f};
            camera.target = {0.0f, 0.0f, 0.0f};
            camera.up = {0.0f, 0.0f, -1.0f};
            camera.fovy = 45.0f;
            camera.projection = CAMERA_PERSPECTIVE;
            break;
    }
}

void CameraModes::HandleMouseLook(Camera& camera) {
    // Handle mouse input for camera rotation
    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
        Vector2 mouseDelta = GetMouseDelta();
        
        // Rotate camera based on mouse movement
        float sensitivity = 0.003f;
        camera.target.x += mouseDelta.x * sensitivity;
        camera.target.y -= mouseDelta.y * sensitivity;
    }
}

void CameraModes::HandleKeyboardMovement(Camera& camera) {
    // Handle keyboard input for camera movement
    float speed = 0.1f;
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3CrossProduct(forward, camera.up);
    
    // Forward/backward movement
    if (IsKeyDown(KEY_W)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(forward, speed));
        camera.target = Vector3Add(camera.target, Vector3Scale(forward, speed));
    }
    if (IsKeyDown(KEY_S)) {
        camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, speed));
        camera.target = Vector3Subtract(camera.target, Vector3Scale(forward, speed));
    }
    
    // Left/right movement
    if (IsKeyDown(KEY_A)) {
        camera.position = Vector3Subtract(camera.position, Vector3Scale(right, speed));
        camera.target = Vector3Subtract(camera.target, Vector3Scale(right, speed));
    }
    if (IsKeyDown(KEY_D)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(right, speed));
        camera.target = Vector3Add(camera.target, Vector3Scale(right, speed));
    }
    
    // Up/down movement
    if (IsKeyDown(KEY_SPACE)) {
        camera.position.y += speed;
        camera.target.y += speed;
    }
    if (IsKeyDown(KEY_LEFT_SHIFT)) {
        camera.position.y -= speed;
        camera.target.y -= speed;
    }
} 