//
// Created by I#Oleg
//

#ifndef CAMERAMODES_H
#define CAMERAMODES_H

#include <raylib.h>
#include <raymath.h>
#include <string>

// Available camera modes for the editor
enum EditorCameraMode {
    FREE = 0,          // Free camera with WASD movement
    ORBITAL = 1,       // Orbital camera around target point
    FIRST_PERSON = 2,  // First-person camera mode
    TOP_DOWN = 3       // Top-down strategic view
};

// Camera modes management class
class CameraModes {
public:
    // Camera update functions for different modes
    static void UpdateFreeCamera(Camera& camera);                    // Update free camera
    static void UpdateOrbitalCamera(Camera& camera, Vector3 target); // Update orbital camera
    static void UpdateFirstPersonCamera(Camera& camera);             // Update first-person camera
    static void UpdateTopDownCamera(Camera& camera);                 // Update top-down camera
    
    // Utility functions
    static std::string GetModeName(EditorCameraMode mode);           // Get mode name as string
    static void SetCameraMode(Camera& camera, EditorCameraMode mode); // Set camera to specific mode
    
private:
    // Input handling functions
    static void HandleMouseLook(Camera& camera);                     // Handle mouse look input
    static void HandleKeyboardMovement(Camera& camera);              // Handle keyboard movement
};

#endif // CAMERAMODES_H 