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
      m_isTransforming(false), m_transformStartPoint({0, 0, 0}), m_lastMouseRayPoint({0, 0, 0}),
      m_transformStartPosition({0, 0, 0}), m_transformStartRotation({0, 0, 0}), m_transformStartScale({1, 1, 1})
{
}

void ToolManager::SetActiveTool(Tool tool)
{
    m_activeTool = tool;

    // Set pending creation for object creation tools
    if (tool == ADD_CUBE || tool == ADD_SPHERE || tool == ADD_CYLINDER || tool == ADD_MODEL)
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
            // Start transform operation
            m_isTransforming = true;
            m_transformStartPosition = selectedObj->GetPosition();
            m_transformStartRotation = selectedObj->GetRotation();
            m_transformStartScale = selectedObj->GetScale();
            
            // Get intersection point with ground plane (Y=0) for move/scale, or use object position for rotate
            if (m_activeTool == MOVE || m_activeTool == SCALE)
            {
                m_transformStartPoint = GetRayGroundIntersection(ray);
            }
            else // ROTATE
            {
                m_transformStartPoint = selectedObj->GetPosition();
            }
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
        // Move object along ground plane
        Vector3 newPoint = GetRayGroundIntersection(ray);
        Vector3 delta = Vector3Subtract(newPoint, m_transformStartPoint);
        Vector3 newPosition = Vector3Add(m_transformStartPosition, delta);
        selectedObj->SetPosition(newPosition);
        m_lastMouseRayPoint = newPoint;
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
        // Scale object based on distance from start point
        Vector3 newPoint = GetRayGroundIntersection(ray);
        float startDistance = Vector3Length(Vector3Subtract(m_transformStartPoint, selectedObj->GetPosition()));
        float newDistance = Vector3Length(Vector3Subtract(newPoint, selectedObj->GetPosition()));
        
        if (startDistance > 0.001f)
        {
            float scaleFactor = newDistance / startDistance;
            Vector3 currentScale = selectedObj->GetScale();
            Vector3 newScale = Vector3Scale(m_transformStartScale, scaleFactor);
            
            // Prevent negative scale
            if (newScale.x > 0.01f && newScale.y > 0.01f && newScale.z > 0.01f)
            {
                selectedObj->SetScale(newScale);
            }
        }
        m_lastMouseRayPoint = newPoint;
        break;
    }
    default:
        break;
    }
}

void ToolManager::EndTransform()
{
    m_isTransforming = false;
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
    default:
        std::cout << "Warning: Attempted to create object with invalid tool" << std::endl;
        return;
    }

    SCENE_CAST(scene).AddObject(newObj);
    std::cout << "Created new object with tool: " << tool << std::endl;
}