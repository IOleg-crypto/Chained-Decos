//
// Created by I#Oleg
//

#ifndef OBJECTTRANSFORM_H
#define OBJECTTRANSFORM_H

#include <raylib.h>
#include <raymath.h>
#include <vector>

// Object transformation and manipulation utilities
class ObjectTransform {
public:
    // Gizmo modes for object transformation
    enum class GizmoMode {
        TRANSLATE,  // Move objects in 3D space
        ROTATE,     // Rotate objects around axes
        SCALE       // Scale objects uniformly or per axis
    };
    
    // Gizmo rendering and interaction
    static void DrawGizmo(Vector3 position, Vector3 scale, Vector3 rotation, GizmoMode mode);  // Draw transformation gizmo
    static bool HandleGizmoInteraction(Vector3& position, Vector3& scale, Vector3& rotation, GizmoMode mode);  // Handle gizmo interaction
    
    // Utility functions for object manipulation
    static Matrix GetTransformMatrix(Vector3 position, Vector3 scale, Vector3 rotation);  // Get transformation matrix
    static Vector3 WorldToScreen(Vector3 worldPos, Camera camera, int screenWidth, int screenHeight);  // Convert world to screen coordinates
    static Vector3 ScreenToWorld(Vector2 screenPos, Camera camera, int screenWidth, int screenHeight);  // Convert screen to world coordinates
    
    // Object picking via ray casting
    static int PickObject(const std::vector<Vector3>& positions, const std::vector<Vector3>& scales, 
                         const std::vector<int>& types, Camera camera, Vector2 mousePos);  // Pick object under mouse cursor
    
private:
    // Gizmo rendering functions
    static void DrawTranslateGizmo(Vector3 position);     // Draw translation gizmo
    static void DrawRotateGizmo(Vector3 position);        // Draw rotation gizmo
    static void DrawScaleGizmo(Vector3 position, Vector3 scale);  // Draw scale gizmo
    
    // Gizmo interaction handlers
    static bool HandleTranslateGizmo(Vector3& position);  // Handle translation interaction
    static bool HandleRotateGizmo(Vector3& rotation);     // Handle rotation interaction
    static bool HandleScaleGizmo(Vector3& scale);         // Handle scaling interaction
};

#endif // OBJECTTRANSFORM_H 