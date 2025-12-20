#include <cfloat>
#include <cmath>
#include <iostream>
#include <raylib.h>
#include <raymath.h>
#include <string>
#include <vector>

#include "components/physics/collision/structures/collisionStructures.h"
#include "editor/Editor.h"
#include "editor/EditorTypes.h"
#include "editor/IEditor.h"
#include "editor/tool/ToolManager.h"
#include "scene/camera/core/CameraController.h"
#include "scene/resources/map/core/MapData.h"

ToolManager::ToolManager()
    : m_activeTool(SELECT), m_pendingObjectCreation(false), m_currentlySelectedModelName(""),
      m_isTransforming(false), m_selectedAxis(GizmoAxis::NONE), m_transformStartPoint({0, 0, 0}),
      m_lastMouseRayPoint({0, 0, 0}), m_transformStartPosition({0, 0, 0}),
      m_transformStartRotation({0, 0, 0}), m_transformStartScale({1, 1, 1}), m_camera({})
{
}

void ToolManager::SetCamera(const Camera3D &camera)
{
    m_camera = camera;
}

float ToolManager::GetGizmoScale(const Vector3 &position) const
{
    Vector3 toCamera = Vector3Subtract(m_camera.position, position);
    float distance = Vector3Length(toCamera);
    float scale = distance * 0.1f; // Scale gizmo based on distance from camera
    if (scale < 0.5f)
        scale = 0.5f;
    if (scale > 2.0f)
        scale = 2.0f;
    return scale;
}

void ToolManager::SetActiveTool(Tool tool)
{
    m_activeTool = tool;

    // Set pending creation for object creation tools
    if (tool == ADD_CUBE || tool == ADD_SPHERE || tool == ADD_CYLINDER || tool == ADD_MODEL ||
        tool == ADD_SPAWN_ZONE)
    {
        m_pendingObjectCreation = true;
    }
}

Tool ToolManager::GetActiveTool() const
{
    return m_activeTool;
}

bool ToolManager::ExecutePendingAction(IEditor &editor)
{
    if (!m_pendingObjectCreation)
    {
        return false;
    }

    CreateObjectForTool(m_activeTool, editor);
    m_pendingObjectCreation = false;
    m_activeTool = SELECT; // Reset to select tool after creation

    return true;
}

void ToolManager::SetSelectedModel(const std::string &modelName)
{
    m_currentlySelectedModelName = modelName;
}

const std::string &ToolManager::GetSelectedModel() const
{
    return m_currentlySelectedModelName;
}

void ToolManager::HandleToolInput(bool mousePressed, const Ray &ray, IEditor &editor)
{
    MapObjectData *selectedObj = editor.GetSelectedObject();

    if (mousePressed)
    {
        if (m_activeTool == SELECT)
        {
            // Select object
            // CollisionRay collisionRay(ray.position, ray.direction);
            // editor.PickObject(collisionRay); // Need to implement PickObject in Editor or use
            // Renderer
        }
        else if ((m_activeTool == MOVE || m_activeTool == ROTATE || m_activeTool == SCALE) &&
                 selectedObj != nullptr)
        {
            // For MOVE and SCALE, check if clicking on a gizmo axis
            if (m_activeTool == MOVE || m_activeTool == SCALE)
            {
                // Calculate gizmo scale based on camera distance
                float scale = GetGizmoScale(selectedObj->position);
                Vector3 gizmoScale = {scale, scale, scale};
                m_selectedAxis = PickGizmoAxis(ray, selectedObj->position, gizmoScale);

                // If no axis was picked, use ground plane intersection
                if (m_selectedAxis == GizmoAxis::NONE)
                {
                    m_transformStartPoint = GetRayGroundIntersection(ray);
                }
                else
                {
                    // Use object position as start point for axis-constrained transforms
                    m_transformStartPoint = selectedObj->position;
                }
            }
            else // ROTATE
            {
                m_selectedAxis = GizmoAxis::NONE;
                m_transformStartPoint = selectedObj->position;
            }

            // Start transform operation
            m_isTransforming = true;
            m_transformStartPosition = selectedObj->position;
            m_transformStartRotation = selectedObj->rotation;
            m_transformStartScale = selectedObj->scale;
            m_lastMouseRayPoint = m_transformStartPoint;
        }
    }
    else if (!mousePressed && m_isTransforming)
    {
        // End transform operation
        EndTransform();
    }
}

void ToolManager::UpdateTool(const Ray &ray, IEditor &editor)
{
    if (!m_isTransforming)
    {
        return;
    }

    MapObjectData *selectedObj = editor.GetSelectedObject();
    if (selectedObj == nullptr)
    {
        EndTransform();
        return;
    }

    switch (m_activeTool)
    {
    case MOVE:
    {
        if (m_selectedAxis != GizmoAxis::NONE)
        {
            // Move along selected axis
            Vector3 newPoint = GetRayGroundIntersection(ray);

            // Project delta onto the selected axis
            Vector3 delta = Vector3Subtract(newPoint, m_transformStartPoint);
            Vector3 axisDir = {0, 0, 0};
            switch (m_selectedAxis)
            {
            case GizmoAxis::X:
                axisDir = {1, 0, 0};
                break;
            case GizmoAxis::Y:
                axisDir = {0, 1, 0};
                break;
            case GizmoAxis::Z:
                axisDir = {0, 0, 1};
                break;
            default:
                break;
            }

            float projection = Vector3DotProduct(delta, axisDir);
            Vector3 newPosition =
                Vector3Add(m_transformStartPosition, Vector3Scale(axisDir, projection));
            selectedObj->position = newPosition;
            editor.SetSceneModified(true);
        }
        else
        {
            // Move object along ground plane (backward compatibility)
            Vector3 newPoint = GetRayGroundIntersection(ray);
            Vector3 delta = Vector3Subtract(newPoint, m_transformStartPoint);
            Vector3 newPosition = Vector3Add(m_transformStartPosition, delta);
            selectedObj->position = newPosition;
            editor.SetSceneModified(true);
        }
        break;
    }
    case ROTATE:
    {
        // Rotate object around its Y axis based on mouse horizontal movement
        Vector2 mouseDelta = GetMouseDelta();
        // Apply filtering to prevent glitches on Linux/VM
        mouseDelta = CameraController::FilterMouseDelta(mouseDelta);
        float rotationSpeed = 0.01f; // Adjust sensitivity
        selectedObj->rotation.y += mouseDelta.x * rotationSpeed;
        editor.SetSceneModified(true);
        break;
    }
    case SCALE:
    {
        if (m_selectedAxis != GizmoAxis::NONE)
        {
            // Scale along selected axis
            Vector3 newPoint = GetRayGroundIntersection(ray);

            // Calculate distance along the selected axis
            Vector3 axisDir = {0, 0, 0};
            switch (m_selectedAxis)
            {
            case GizmoAxis::X:
                axisDir = {1, 0, 0};
                break;
            case GizmoAxis::Y:
                axisDir = {0, 1, 0};
                break;
            case GizmoAxis::Z:
                axisDir = {0, 0, 1};
                break;
            default:
                break;
            }

            float startDistance = Vector3DotProduct(
                Vector3Subtract(m_transformStartPoint, selectedObj->position), axisDir);
            float newDistance =
                Vector3DotProduct(Vector3Subtract(newPoint, selectedObj->position), axisDir);

            if (fabs(startDistance) > 0.001f)
            {
                float scaleFactor = newDistance / startDistance;
                Vector3 newScale = Vector3Scale(m_transformStartScale, 1.0f);

                switch (m_selectedAxis)
                {
                case GizmoAxis::X:
                    newScale.x *= scaleFactor;
                    break;
                case GizmoAxis::Y:
                    newScale.y *= scaleFactor;
                    break;
                case GizmoAxis::Z:
                    newScale.z *= scaleFactor;
                    break;
                default:
                    break;
                }

                // Prevent negative scale
                selectedObj->scale = newScale;
                editor.SetSceneModified(true);
            }
        }
        else
        {
            // Scale object based on distance from start point (backward compatibility)
            Vector3 newPoint = GetRayGroundIntersection(ray);
            float startDistance =
                Vector3Length(Vector3Subtract(m_transformStartPoint, selectedObj->position));
            float newDistance = Vector3Length(Vector3Subtract(newPoint, selectedObj->position));

            if (startDistance > 0.001f)
            {
                float scaleFactor = newDistance / startDistance;
                Vector3 newScale = Vector3Scale(m_transformStartScale, scaleFactor);

                // Prevent negative scale
                if (newScale.x > 0.01f && newScale.y > 0.01f && newScale.z > 0.01f)
                {
                    selectedObj->scale = newScale;
                    editor.SetSceneModified(true);
                }
            }
        }
        break;
    }
    default:
        break;
    }
}

void ToolManager::EndTransform()
{
    m_isTransforming = false;
    m_selectedAxis = GizmoAxis::NONE;
    m_transformStartPoint = {0, 0, 0};
    m_lastMouseRayPoint = {0, 0, 0};
}

Vector3 ToolManager::GetRayPlaneIntersection(const Ray &ray, const Vector3 &planePoint,
                                             const Vector3 &planeNormal)
{
    float denom = Vector3DotProduct(planeNormal, ray.direction);
    if (fabs(denom) > 0.0001f)
    {
        Vector3 toPlane = Vector3Subtract(planePoint, ray.position);
        float t = Vector3DotProduct(toPlane, planeNormal) / denom;
        return Vector3Add(ray.position, Vector3Scale(ray.direction, t));
    }
    return ray.position;
}

Vector3 ToolManager::GetRayGroundIntersection(const Ray &ray)
{
    Vector3 groundPoint = {0, 0, 0};
    Vector3 groundNormal = {0, 1, 0}; // Y-up
    return GetRayPlaneIntersection(ray, groundPoint, groundNormal);
}

Vector3 ToolManager::GetClosestPointOnRay(const Vector3 &point, const Vector3 &rayStart,
                                          const Vector3 &rayDir)
{
    Vector3 toPoint = Vector3Subtract(point, rayStart);
    float t = Vector3DotProduct(toPoint, rayDir);
    return Vector3Add(rayStart, Vector3Scale(rayDir, t));
}

GizmoAxis ToolManager::PickGizmoAxis(const Ray &ray, const Vector3 &objPos,
                                     const Vector3 &gizmoScale)
{
    float gizmoLength = 2.0f;
    float arrowLength = gizmoLength * gizmoScale.x;

    // Define gizmo axes (these match what we render in Editor::RenderGizmo)
    Vector3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; // X, Y, Z
    GizmoAxis axisEnums[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};

    float pickThreshold = 0.3f; // Maximum distance from axis to consider a pick
    float minDistance = FLT_MAX;
    GizmoAxis closestAxis = GizmoAxis::NONE;

    for (int i = 0; i < 3; i++)
    {
        // For each axis, find the closest point on the axis to the ray
        // We need to find the shortest distance between two lines (ray and axis)

        Vector3 axisDir = axes[i];
        Vector3 axisStart = objPos;
        Vector3 axisEnd = Vector3Add(objPos, Vector3Scale(axisDir, arrowLength));

        // Calculate closest points between ray and axis line segment
        Vector3 rayDir = ray.direction;
        Vector3 w0 = Vector3Subtract(ray.position, axisStart);

        float a = Vector3DotProduct(rayDir, rayDir);
        float b = Vector3DotProduct(rayDir, axisDir);
        float c = Vector3DotProduct(axisDir, axisDir);
        float d = Vector3DotProduct(rayDir, w0);
        float e = Vector3DotProduct(axisDir, w0);

        float denom = a * c - b * b;
        float t_ray, t_axis;

        if (fabs(denom) < 0.0001f)
        {
            // Lines are parallel
            t_ray = 0.0f;
            t_axis = (b > c ? d / b : e / c);
        }
        else
        {
            t_ray = (b * e - c * d) / denom;
            t_axis = (a * e - b * d) / denom;
        }

        // Clamp t_axis to axis segment [0, arrowLength]
        t_axis = fmaxf(0.0f, fminf(arrowLength, t_axis));

        // Calculate closest points
        Vector3 pointOnRay = Vector3Add(ray.position, Vector3Scale(rayDir, t_ray));
        Vector3 pointOnAxis = Vector3Add(axisStart, Vector3Scale(axisDir, t_axis));

        // Distance between the two closest points
        float distance = Vector3Distance(pointOnRay, pointOnAxis);

        // Check if this is the closest axis and within threshold
        if (distance < minDistance && distance < pickThreshold)
        {
            minDistance = distance;
            closestAxis = axisEnums[i];
        }
    }

    return closestAxis;
}

void ToolManager::CreateObjectForTool(Tool tool, IEditor &editor)
{
    MapObjectType type;
    switch (tool)
    {
    case ADD_CUBE:
        type = MapObjectType::CUBE;
        break;
    case ADD_SPHERE:
        type = MapObjectType::SPHERE;
        break;
    case ADD_CYLINDER:
        type = MapObjectType::CYLINDER;
        break;
    case ADD_MODEL:
        type = MapObjectType::MODEL;
        break;
    case ADD_SPAWN_ZONE:
        type = MapObjectType::SPAWN_ZONE;
        break;
    default:
        std::cout << "Warning: Attempted to create object with invalid tool" << std::endl;
        return;
    }

    dynamic_cast<Editor &>(editor).CreateDefaultObject(type, m_currentlySelectedModelName);
    std::cout << "Created new object with tool: " << (int)tool << std::endl;
}

void ToolManager::RenderGizmos(IEditor &editor)
{
    MapObjectData *selectedObj = editor.GetSelectedObject();
    if (!selectedObj)
        return;

    // Only render gizmos for transform tools
    if (m_activeTool != MOVE && m_activeTool != ROTATE && m_activeTool != SCALE)
        return;

    // Calculate gizmo scale based on camera distance
    float scale = GetGizmoScale(selectedObj->position);
    Vector3 gizmoScale = {scale, scale, scale};

    // Render based on active tool
    if (m_activeTool == MOVE)
    {
        // Render Move Gizmo (Arrows)
        DrawCube(selectedObj->position, 0.2f * scale, 0.2f * scale, 0.2f * scale, WHITE);

        // X Axis (Red)
        DrawLine3D(selectedObj->position,
                   Vector3Add(selectedObj->position, Vector3Scale({1, 0, 0}, 2.0f * scale)), RED);
        DrawCube(Vector3Add(selectedObj->position, Vector3Scale({1, 0, 0}, 2.0f * scale)),
                 0.3f * scale, 0.3f * scale, 0.3f * scale, RED);

        // Y Axis (Green)
        DrawLine3D(selectedObj->position,
                   Vector3Add(selectedObj->position, Vector3Scale({0, 1, 0}, 2.0f * scale)), GREEN);
        DrawCube(Vector3Add(selectedObj->position, Vector3Scale({0, 1, 0}, 2.0f * scale)),
                 0.3f * scale, 0.3f * scale, 0.3f * scale, GREEN);

        // Z Axis (Blue)
        DrawLine3D(selectedObj->position,
                   Vector3Add(selectedObj->position, Vector3Scale({0, 0, 1}, 2.0f * scale)), BLUE);
        DrawCube(Vector3Add(selectedObj->position, Vector3Scale({0, 0, 1}, 2.0f * scale)),
                 0.3f * scale, 0.3f * scale, 0.3f * scale, BLUE);
    }
    else if (m_activeTool == SCALE)
    {
        // Render Scale Gizmo (Cubes)
        DrawCube(selectedObj->position, 0.2f * scale, 0.2f * scale, 0.2f * scale, WHITE);

        // X Axis (Red)
        DrawLine3D(selectedObj->position,
                   Vector3Add(selectedObj->position, Vector3Scale({1, 0, 0}, 2.0f * scale)), RED);
        DrawCube(Vector3Add(selectedObj->position, Vector3Scale({1, 0, 0}, 2.0f * scale)),
                 0.3f * scale, 0.3f * scale, 0.3f * scale, RED);

        // Y Axis (Green)
        DrawLine3D(selectedObj->position,
                   Vector3Add(selectedObj->position, Vector3Scale({0, 1, 0}, 2.0f * scale)), GREEN);
        DrawCube(Vector3Add(selectedObj->position, Vector3Scale({0, 1, 0}, 2.0f * scale)),
                 0.3f * scale, 0.3f * scale, 0.3f * scale, GREEN);

        // Z Axis (Blue)
        DrawLine3D(selectedObj->position,
                   Vector3Add(selectedObj->position, Vector3Scale({0, 0, 1}, 2.0f * scale)), BLUE);
        DrawCube(Vector3Add(selectedObj->position, Vector3Scale({0, 0, 1}, 2.0f * scale)),
                 0.3f * scale, 0.3f * scale, 0.3f * scale, BLUE);
    }
    else if (m_activeTool == ROTATE)
    {
        // Render Rotate Gizmo (Circles) - simplified
        DrawCircle3D(selectedObj->position, 2.0f * scale, {1, 0, 0}, 90.0f, RED);
        DrawCircle3D(selectedObj->position, 2.0f * scale, {0, 1, 0}, 90.0f, GREEN);
        DrawCircle3D(selectedObj->position, 2.0f * scale, {0, 0, 1}, 90.0f, BLUE);
    }
}


