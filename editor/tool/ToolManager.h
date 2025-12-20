#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include "editor/EditorTypes.h"
#include "editor/IEditor.h"
#include "editor/tool/IToolManager.h"
#include <cstdint>
#include <raylib.h>
#include <string>
#include <vector>

class ToolManager : public IToolManager
{
private:
    Tool m_activeTool;
    bool m_pendingObjectCreation;
    std::string m_currentlySelectedModelName;

    // Gizmo state
    bool m_isTransforming;
    GizmoAxis m_selectedAxis;
    Vector3 m_transformStartPoint;
    Vector3 m_lastMouseRayPoint;
    Vector3 m_transformStartPosition;
    Vector3 m_transformStartRotation;
    Vector3 m_transformStartScale;

    Camera3D m_camera;

public:
    ToolManager();
    ~ToolManager() override = default;

    void SetCamera(const Camera3D &camera) override;
    void SetActiveTool(Tool tool) override;
    Tool GetActiveTool() const override;

    bool ExecutePendingAction(IEditor &editor) override;
    void SetSelectedModel(const std::string &modelName) override;
    const std::string &GetSelectedModel() const override;

    void HandleToolInput(bool mousePressed, const Ray &ray, IEditor &editor) override;
    void UpdateTool(const Ray &ray, IEditor &editor) override;
    void RenderGizmos(IEditor &editor) override;

private:
    float GetGizmoScale(const Vector3 &position) const;
    GizmoAxis PickGizmoAxis(const Ray &ray, const Vector3 &position, const Vector3 &scale);
    void DrawGizmo(const Vector3 &position, const Vector3 &scale, GizmoAxis selectedAxis);
    void EndTransform();
    Vector3 GetRayPlaneIntersection(const Ray &ray, const Vector3 &planePoint,
                                    const Vector3 &planeNormal);
    Vector3 GetRayGroundIntersection(const Ray &ray);
    Vector3 GetClosestPointOnRay(const Vector3 &point, const Vector3 &rayStart,
                                 const Vector3 &rayDir);
    void CreateObjectForTool(Tool tool, IEditor &editor);
};

#endif // TOOLMANAGER_H



