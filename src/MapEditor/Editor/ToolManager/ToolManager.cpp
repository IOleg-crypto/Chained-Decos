#include "ToolManager.h"
#include "../SceneManager/SceneManager.h"
#include "../Object/MapObject.h"
#include "Engine/Collision/CollisionStructures.h"
#include <iostream>
#include <cmath>

// Cast ISceneManager to SceneManager for implementation
#define SCENE_CAST(scene) dynamic_cast<SceneManager&>(scene)

ToolManager::ToolManager()
    : m_activeTool(SELECT), m_pendingObjectCreation(false), m_currentlySelectedModelName(""),
      m_isTransforming(false), m_selectedAxis(GizmoAxis::NONE), m_transformStartPoint({0, 0, 0}), m_lastMouseRayPoint({0, 0, 0}),
      m_transformStartPosition({0, 0, 0}), m_transformStartRotation({0, 0, 0}), m_transformStartScale({1, 1, 1})
{
}

void ToolManager::SetActiveTool(Tool tool)
{
    m_activeTool = tool;

    // Set pending creation for object creation tools
    if (tool == ADD_CUBE || tool == ADD_SPHERE || tool == ADD_CYLINDER || tool == ADD_MODEL || tool == ADD_SPAWN_ZONE)
    {
        m_pendingObjectCreation = true;
    }
}

Tool ToolManager::GetActiveTool() const
{
    return m_activeTool;
}

bool ToolManager::ExecutePendingAction(ISceneManager& scene)
{
    if (!m_pendingObjectCreation)
    {
        return false;
    }

    CreateObjectForTool(m_activeTool, SCENE_CAST(scene));
    m_pendingObjectCreation = false;
    m_activeTool = SELECT; // Reset to select tool after creation

    return true;
}

void ToolManager::SetSelectedModel(const std::string& modelName)
{
    m_currentlySelectedModelName = modelName;
}

const std::string& ToolManager::GetSelectedModel() const
{
    return m_currentlySelectedModelName;
}

void ToolManager::HandleToolInput(bool mousePressed, const Ray& ray, ISceneManager& scene)
{
    MapObject* selectedObj = SCENE_CAST(scene).GetSelectedObject();
    
    if (mousePressed)
    {
        if (m_activeTool == SELECT)
        {
            // Select object
            CollisionRay collisionRay(ray.position, ray.direction);
            SCENE_CAST(scene).PickObject(collisionRay);
        }
        else if ((m_activeTool == MOVE || m_activeTool == ROTATE || m_activeTool == SCALE) && selectedObj != nullptr)
        {
            // For MOVE and SCALE, check if clicking on a gizmo axis
            if (m_activeTool == MOVE || m_activeTool == SCALE)
            {
                // Calculate gizmo scale (simplified, should ideally come from camera)
                Vector3 gizmoScale = {1.0f, 1.0f, 1.0f};
                m_selectedAxis = PickGizmoAxis(ray, selectedObj->GetPosition(), gizmoScale);
                
                // If no axis was picked, use ground plane intersection
                if (m_selectedAxis == GizmoAxis::NONE)
                {
                    m_transformStartPoint = GetRayGroundIntersection(ray);
                }
                else
                {
                    // Use object position as start point for axis-constrained transforms
                    m_transformStartPoint = selectedObj->GetPosition();
                }
            }
            else // ROTATE
            {
                m_selectedAxis = GizmoAxis::NONE;
                m_transformStartPoint = selectedObj->GetPosition();
            }
            
            // Start transform operation
            m_isTransforming = true;
            m_transformStartPosition = selectedObj->GetPosition();
            m_transformStartRotation = selectedObj->GetRotation();
            m_transformStartScale = selectedObj->GetScale();
            m_lastMouseRayPoint = m_transformStartPoint;
        }
    }
    else if (!mousePressed && m_isTransforming)
    {
        // End transform operation
        EndTransform();
    }
}

void ToolManager::UpdateTool(const Ray& ray, ISceneManager& scene)
{
    if (!m_isTransforming)
    {
        return;
    }
    
    MapObject* selectedObj = SCENE_CAST(scene).GetSelectedObject();
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
                case GizmoAxis::X: axisDir = {1, 0, 0}; break;
                case GizmoAxis::Y: axisDir = {0, 1, 0}; break;
                case GizmoAxis::Z: axisDir = {0, 0, 1}; break;
                default: break;
            }
            
            float projection = Vector3DotProduct(delta, axisDir);
            Vector3 newPosition = Vector3Add(m_transformStartPosition, Vector3Scale(axisDir, projection));
            selectedObj->SetPosition(newPosition);
        }
        else
        {
            // Move object along ground plane (backward compatibility)
            Vector3 newPoint = GetRayGroundIntersection(ray);
            Vector3 delta = Vector3Subtract(newPoint, m_transformStartPoint);
            Vector3 newPosition = Vector3Add(m_transformStartPosition, delta);
            selectedObj->SetPosition(newPosition);
        }
        break;
    }
    case ROTATE:
    {
        // Rotate object around its Y axis based on mouse horizontal movement
        Vector2 mouseDelta = GetMouseDelta();
        float rotationSpeed = 0.01f; // Adjust sensitivity
        Vector3 currentRotation = selectedObj->GetRotation();
        currentRotation.y += mouseDelta.x * rotationSpeed;
        selectedObj->SetRotation(currentRotation);
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
                case GizmoAxis::X: axisDir = {1, 0, 0}; break;
                case GizmoAxis::Y: axisDir = {0, 1, 0}; break;
                case GizmoAxis::Z: axisDir = {0, 0, 1}; break;
                default: break;
            }
            
            float startDistance = Vector3DotProduct(Vector3Subtract(m_transformStartPoint, selectedObj->GetPosition()), axisDir);
            float newDistance = Vector3DotProduct(Vector3Subtract(newPoint, selectedObj->GetPosition()), axisDir);
            
            if (fabs(startDistance) > 0.001f)
            {
                float scaleFactor = newDistance / startDistance;
                Vector3 newScale = Vector3Scale(m_transformStartScale, 1.0f);
                
                switch (m_selectedAxis)
                {
                    case GizmoAxis::X: newScale.x *= scaleFactor; break;
                    case GizmoAxis::Y: newScale.y *= scaleFactor; break;
                    case GizmoAxis::Z: newScale.z *= scaleFactor; break;
                    default: break;
                }
                
                // Prevent negative scale
                if (newScale.x > 0.01f && newScale.y > 0.01f && newScale.z > 0.01f)
                {
                    selectedObj->SetScale(newScale);
                }
            }
        }
        else
        {
            // Scale object based on distance from start point (backward compatibility)
            Vector3 newPoint = GetRayGroundIntersection(ray);
            float startDistance = Vector3Length(Vector3Subtract(m_transformStartPoint, selectedObj->GetPosition()));
            float newDistance = Vector3Length(Vector3Subtract(newPoint, selectedObj->GetPosition()));
            
            if (startDistance > 0.001f)
            {
                float scaleFactor = newDistance / startDistance;
                Vector3 newScale = Vector3Scale(m_transformStartScale, scaleFactor);
                
                // Prevent negative scale
                if (newScale.x > 0.01f && newScale.y > 0.01f && newScale.z > 0.01f)
                {
                    selectedObj->SetScale(newScale);
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

Vector3 ToolManager::GetRayPlaneIntersection(const Ray& ray, const Vector3& planePoint, const Vector3& planeNormal)
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

Vector3 ToolManager::GetRayGroundIntersection(const Ray& ray)
{
    Vector3 groundPoint = {0, 0, 0};
    Vector3 groundNormal = {0, 1, 0}; // Y-up
    return GetRayPlaneIntersection(ray, groundPoint, groundNormal);
}

Vector3 ToolManager::GetClosestPointOnRay(const Vector3& point, const Vector3& rayStart, const Vector3& rayDir)
{
    Vector3 toPoint = Vector3Subtract(point, rayStart);
    float t = Vector3DotProduct(toPoint, rayDir);
    return Vector3Add(rayStart, Vector3Scale(rayDir, t));
}

GizmoAxis ToolManager::PickGizmoAxis(const Ray& ray, const Vector3& objPos, const Vector3& gizmoScale)
{
    float gizmoLength = 2.0f;
    float arrowLength = gizmoLength * gizmoScale.x;
    
    // Define gizmo axes (these match what we render in Editor::RenderGizmo)
    Vector3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; // X, Y, Z
    GizmoAxis axisEnums[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
    
    float minDistance = 0.5f * arrowLength * 0.7f; // Within 70% of arrow length
    GizmoAxis closestAxis = GizmoAxis::NONE;
    
    for (int i = 0; i < 3; i++)
    {
        Vector3 axisEnd = Vector3Add(objPos, Vector3Scale(axes[i], arrowLength));
        Vector3 closestPointOnAxis = GetClosestPointOnRay(ray.position, objPos, axes[i]);
        
        // Check if closest point is within the arrow range
        float distOnAxis = Vector3Length(Vector3Subtract(closestPointOnAxis, objPos));
        if (distOnAxis < 0.0f || distOnAxis > arrowLength) continue;
        
        // Calculate distance from ray start to closest point
        float distToAxis = Vector3Distance(ray.position, closestPointOnAxis);
        
        if (distToAxis < minDistance)
        {
            minDistance = distToAxis;
            closestAxis = axisEnums[i];
        }
    }
    
    return closestAxis;
}

void ToolManager::CreateObjectForTool(Tool tool, ISceneManager& scene)
{
    MapObject newObj;
    std::string baseName = "New Object " + std::to_string(SCENE_CAST(scene).GetObjects().size());

    switch (tool)
    {
    case ADD_CUBE:
        newObj.SetObjectType(0); // Cube
        newObj.SetObjectName(baseName + " (Cube)");
        break;
    case ADD_SPHERE:
        newObj.SetObjectType(1); // Sphere
        newObj.SetObjectName(baseName + " (Sphere)");
        break;
    case ADD_CYLINDER:
        newObj.SetObjectType(2); // Cylinder
        newObj.SetObjectName(baseName + " (Cylinder)");
        break;
    case ADD_MODEL:
        newObj.SetObjectType(5); // Model
        newObj.SetModelAssetName(m_currentlySelectedModelName);
        newObj.SetObjectName(m_currentlySelectedModelName + " " + std::to_string(SCENE_CAST(scene).GetObjects().size()));
        break;
    case ADD_SPAWN_ZONE:
        newObj.SetObjectType(6); // Spawn Zone
        newObj.SetObjectName("Spawn Zone");
        newObj.SetColor({255, 100, 100, 200}); // Semi-transparent red
        break;
    default:
        std::cout << "Warning: Attempted to create object with invalid tool" << std::endl;
        return;
    }

    SCENE_CAST(scene).AddObject(newObj);
    std::cout << "Created new object with tool: " << tool << std::endl;
}