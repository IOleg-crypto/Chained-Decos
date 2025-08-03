//
// Created by I#Oleg
//

#include "ObjectTransform.h"
#include <cmath>

void ObjectTransform::DrawGizmo(Vector3 position, Vector3 scale, Vector3 rotation, GizmoMode mode) {
    // Draw appropriate gizmo based on mode
    switch (mode) {
        case GizmoMode::TRANSLATE:
            DrawTranslateGizmo(position);
            break;
        case GizmoMode::ROTATE:
            DrawRotateGizmo(position);
            break;
        case GizmoMode::SCALE:
            DrawScaleGizmo(position, scale);
            break;
    }
}

bool ObjectTransform::HandleGizmoInteraction(Vector3& position, Vector3& scale, Vector3& rotation, GizmoMode mode) {
    // Handle interaction based on gizmo mode
    switch (mode) {
        case GizmoMode::TRANSLATE:
            return HandleTranslateGizmo(position);
        case GizmoMode::ROTATE:
            return HandleRotateGizmo(rotation);
        case GizmoMode::SCALE:
            return HandleScaleGizmo(scale);
        default:
            return false;
    }
}

Matrix ObjectTransform::GetTransformMatrix(Vector3 position, Vector3 scale, Vector3 rotation) {
    // Create transformation matrix from position, scale, and rotation
    Matrix translation = MatrixTranslate(position.x, position.y, position.z);
    Matrix scaling = MatrixScale(scale.x, scale.y, scale.z);
    Matrix rotationX = MatrixRotateX(rotation.x);
    Matrix rotationY = MatrixRotateY(rotation.y);
    Matrix rotationZ = MatrixRotateZ(rotation.z);
    
    // Combine transformations: translate * rotate * scale
    return MatrixMultiply(MatrixMultiply(MatrixMultiply(translation, rotationZ), 
                                       MatrixMultiply(rotationY, rotationX)), scaling);
}

Vector3 ObjectTransform::WorldToScreen(Vector3 worldPos, Camera camera, int screenWidth, int screenHeight) {
    // Convert 3D world position to 2D screen coordinates
    Vector2 screenPos = GetWorldToScreen(worldPos, camera);
    return {screenPos.x, screenPos.y};
}

Vector3 ObjectTransform::ScreenToWorld(Vector2 screenPos, Camera camera, int screenWidth, int screenHeight) {
    // Convert 2D screen position to 3D world ray
    auto [position, direction] = GetMouseRay(screenPos, camera);
    return position;
}

int ObjectTransform::PickObject(const std::vector<Vector3>& positions, const std::vector<Vector3>& scales, 
                              const std::vector<int>& types, Camera camera, Vector2 mousePos) {
    // Cast ray from mouse position and find closest object
    Ray ray = GetMouseRay(mousePos, camera);
    float closestDistance = INFINITY;
    int closestObject = -1;
    
    // Check each object for intersection
    for (int i = 0; i < positions.size(); i++) {
        BoundingBox bbox = {};
        
        // Create bounding box based on object type
        switch (types[i]) {
            case 0: // Cube - use scale as box dimensions
                bbox.min = Vector3Subtract(positions[i], Vector3Scale(scales[i], 0.5f));
                bbox.max = Vector3Add(positions[i], Vector3Scale(scales[i], 0.5f));
                break;
            case 1: // Sphere - use scale.x as radius
                bbox.min = Vector3Subtract(positions[i], {scales[i].x, scales[i].x, scales[i].x});
                bbox.max = Vector3Add(positions[i], {scales[i].x, scales[i].x, scales[i].x});
                break;
            case 2: // Cylinder - use scale.x as radius, scale.y as height
                bbox.min = Vector3Subtract(positions[i], {scales[i].x, scales[i].y, scales[i].x});
                bbox.max = Vector3Add(positions[i], {scales[i].x, scales[i].y, scales[i].x});
                break;
        }
        
        // Check ray-box intersection
        RayCollision collision = GetRayCollisionBox(ray, bbox);
        if (collision.hit && collision.distance < closestDistance) {
            closestDistance = collision.distance;
            closestObject = i;
        }
    }
    
    return closestObject;
}

void ObjectTransform::DrawTranslateGizmo(Vector3 position) {
    // Draw translation gizmo with colored axes
    float size = 1.0f;
    Color colors[] = {RED, GREEN, BLUE};  // X, Y, Z axis colors
    Vector3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};  // Axis directions
    
    // Draw each axis line with handle
    for (int i = 0; i < 3; i++) {
        Vector3 end = Vector3Add(position, Vector3Scale(axes[i], size));
        DrawLine3D(position, end, colors[i]);
        DrawCube(end, 0.1f, 0.1f, 0.1f, colors[i]);
    }
}

void ObjectTransform::DrawRotateGizmo(Vector3 position) {
    // Draw rotation gizmo with circular handles
    constexpr Color colors[] = {RED, GREEN, BLUE};  // X, Y, Z rotation colors
    
    // Draw circles for each rotation axis
    for (int axis = 0; axis < 3; axis++) {
        for (int i = 0; i < 32; i++) {
            float radius = 1.0f;
            float angle1 = static_cast<float>(i) * 2.0f * PI / 32.0f;
            float angle2 = static_cast<float>(i + 1) * 2.0f * PI / 32.0f;
            
            Vector3 p1 = {}, p2 = {};
            switch (axis) {
                case 0: // X axis circle (YZ plane)
                    p1 = {position.x, position.y + radius * cos(angle1), position.z + radius * sin(angle1)};
                    p2 = {position.x, position.y + radius * cos(angle2), position.z + radius * sin(angle2)};
                    break;
                case 1: // Y axis circle (XZ plane)
                    p1 = {position.x + radius * cos(angle1), position.y, position.z + radius * sin(angle1)};
                    p2 = {position.x + radius * cos(angle2), position.y, position.z + radius * sin(angle2)};
                    break;
                case 2: // Z axis circle (XY plane)
                    p1 = {position.x + radius * cos(angle1), position.y + radius * sin(angle1), position.z};
                    p2 = {position.x + radius * cos(angle2), position.y + radius * sin(angle2), position.z};
                    break;
                default: ;
            }
            DrawLine3D(p1, p2, colors[axis]);
        }
    }
}

void ObjectTransform::DrawScaleGizmo(Vector3 position, Vector3 scale) {
    // Draw scale gizmo with axis handles
    float size = 1.0f;
    Color colors[] = {RED, GREEN, BLUE};  // X, Y, Z scale colors
    Vector3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};  // Axis directions
    
    // Draw each scale axis
    for (int i = 0; i < 3; i++) {
        Vector3 end = Vector3Add(position, Vector3Scale(axes[i], size));
        DrawLine3D(position, end, colors[i]);
        DrawCube(end, 0.1f, 0.1f, 0.1f, colors[i]);
    }
    
    // Draw center cube for uniform scaling
    DrawCubeWires(position, 0.2f, 0.2f, 0.2f, WHITE);
}

bool ObjectTransform::HandleTranslateGizmo(Vector3& position) {
    // TODO: Implement interactive translation
    // This would handle mouse drag on translation handles
    return false;
}

bool ObjectTransform::HandleRotateGizmo(Vector3& rotation) {
    // TODO: Implement interactive rotation
    // This would handle mouse drag on rotation circles
    return false;
}

bool ObjectTransform::HandleScaleGizmo(Vector3& scale) {
    // TODO: Implement interactive scaling
    // This would handle mouse drag on scale handles
    return false;
} 