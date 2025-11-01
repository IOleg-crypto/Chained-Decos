#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include "IToolManager.h"
#include <string>

class ToolManager : public IToolManager {
private:
    Tool m_activeTool;
    bool m_pendingObjectCreation;
    std::string m_currentlySelectedModelName;
    
    // State for transform operations (Move, Rotate, Scale)
    bool m_isTransforming;
    Vector3 m_transformStartPoint;  // Starting point for transformation
    Vector3 m_lastMouseRayPoint;    // Last mouse ray intersection point
    Vector3 m_transformStartPosition; // Object position when transform started
    Vector3 m_transformStartRotation; // Object rotation when transform started
    Vector3 m_transformStartScale;    // Object scale when transform started

public:
    ToolManager();
    ~ToolManager() override = default;

    void SetActiveTool(Tool tool) override;
    Tool GetActiveTool() const override;
    bool ExecutePendingAction(ISceneManager& scene) override;
    void SetSelectedModel(const std::string& modelName) override;
    const std::string& GetSelectedModel() const override;
    void HandleToolInput(bool mousePressed, const Ray& ray, ISceneManager& scene) override;
    
    // Update tool during drag operations
    void UpdateTool(const Ray& ray, ISceneManager& scene);
    void EndTransform();

private:
    void CreateObjectForTool(Tool tool, ISceneManager& scene);
    Vector3 GetRayPlaneIntersection(const Ray& ray, const Vector3& planePoint, const Vector3& planeNormal);
    Vector3 GetRayGroundIntersection(const Ray& ray);
};

#endif // TOOLMANAGER_H