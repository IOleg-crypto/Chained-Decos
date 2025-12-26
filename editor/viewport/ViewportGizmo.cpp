#include "ViewportGizmo.h"
#include "editor/logic/undo/CommandHistory.h"
#include "editor/logic/undo/TransformCommand.h"
#include "editor/panels/ViewportPanel.h"
#include "scene/resources/map/GameScene.h"
#include <raymath.h>
#include <rlgl.h>

namespace CHEngine
{

// Internal helper for plane collision
static RayCollision MyGetRayCollisionPlane(Ray ray, Vector3 planePos, Vector3 planeNormal)
{
    RayCollision collision = {0};
    float denom = Vector3DotProduct(planeNormal, ray.direction);
    if (fabsf(denom) > 0.0001f)
    {
        float t = Vector3DotProduct(Vector3Subtract(planePos, ray.position), planeNormal) / denom;
        if (t >= 0)
        {
            collision.hit = true;
            collision.distance = t;
            collision.point = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
            collision.normal = planeNormal;
        }
    }
    return collision;
}

bool ViewportGizmo::RenderAndHandle(const std::shared_ptr<GameScene> &scene, const Camera3D &camera,
                                    int selectedObjectIndex, Tool currentTool, ImVec2 viewportSize,
                                    bool isHovered, CommandHistory *history)
{
    if (!scene || selectedObjectIndex < 0 ||
        selectedObjectIndex >= (int)scene->GetMapObjects().size())
        return false;

    auto &objects = scene->GetMapObjectsMutable();
    auto &obj = objects[selectedObjectIndex];

    // Reset gizmo hover state
    m_GizmoHovered = false;

    // Create ray for gizmo interaction
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    // Hazel-style: proper NDC conversion with Y-flip for OpenGL
    Vector2 localMouse = {mousePos.x - viewportPos.x, mousePos.y - viewportPos.y};
    float ndcX = (2.0f * localMouse.x) / viewportSize.x - 1.0f;
    float ndcY = 1.0f - (2.0f * localMouse.y) / viewportSize.y;

    // Calculate ray direction in view space
    float aspectRatio = viewportSize.x / viewportSize.y;
    float tanHalfFovy = tanf(camera.fovy * 0.5f * DEG2RAD);

    Vector3 rayDir = {ndcX * aspectRatio * tanHalfFovy, ndcY * tanHalfFovy, 1.0f};

    // Transform to world space
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    Vector3 worldRayDir =
        Vector3Normalize({rayDir.x * right.x + rayDir.y * up.x + rayDir.z * forward.x,
                          rayDir.x * right.y + rayDir.y * up.y + rayDir.z * forward.y,
                          rayDir.x * right.z + rayDir.y * up.z + rayDir.z * forward.z});

    Ray ray = {camera.position, worldRayDir};

    // Handle dragging
    if (m_DraggingAxis != GizmoAxis::NONE)
    {
        if (!ImGui::IsMouseDown(0))
        {
            // Drag finished - push command if state changed
            if (history && (obj.position.x != m_OriginalObjectData.position.x ||
                            obj.position.y != m_OriginalObjectData.position.y ||
                            obj.position.z != m_OriginalObjectData.position.z ||
                            obj.rotation.x != m_OriginalObjectData.rotation.x ||
                            obj.rotation.y != m_OriginalObjectData.rotation.y ||
                            obj.rotation.z != m_OriginalObjectData.rotation.z ||
                            obj.scale.x != m_OriginalObjectData.scale.x ||
                            obj.scale.y != m_OriginalObjectData.scale.y ||
                            obj.scale.z != m_OriginalObjectData.scale.z))
            {
                auto cmd = std::make_unique<TransformCommand>(scene, selectedObjectIndex,
                                                              m_OriginalObjectData, obj);
                history->PushCommand(std::move(cmd));
            }

            m_DraggingAxis = GizmoAxis::NONE;
        }
        else
        {
            HandleGizmoDrag(obj, currentTool, mousePos, camera);
        }
    }

    // Draw handles (and check for picking)
    float gizmoSize = 2.0f;
    float handleRadius = 0.2f;
    float lineThickness = 0.05f;

    auto drawAxisHandle = [&](GizmoAxis axis, Vector3 direction, Color color)
    {
        Vector3 endPos = Vector3Add(obj.position, Vector3Scale(direction, gizmoSize));

        // Interaction logic (Collision detection)
        BoundingBox handleBox = {Vector3Subtract(endPos, {0.3f, 0.3f, 0.3f}),
                                 Vector3Add(endPos, {0.3f, 0.3f, 0.3f})};
        RayCollision handleColl = GetRayCollisionBox(ray, handleBox);

        // Line collision
        RayCollision lineColl = GetRayCollisionBox(
            ray, {Vector3Subtract(Vector3Min(obj.position, endPos), {0.1f, 0.1f, 0.1f}),
                  Vector3Add(Vector3Max(obj.position, endPos), {0.1f, 0.1f, 0.1f})});

        bool hovered = (m_DraggingAxis == axis) || handleColl.hit ||
                       (lineColl.hit && lineColl.distance < 10.0f);

        if (hovered)
            m_GizmoHovered = true;

        if (hovered && ImGui::IsMouseClicked(0) && m_DraggingAxis == GizmoAxis::NONE)
        {
            m_DraggingAxis = axis;
            m_InitialMousePos = mousePos;
            m_OriginalObjectData = obj; // Capture original state

            if (currentTool == Tool::MOVE)
                m_InitialObjectValue = obj.position;
            else if (currentTool == Tool::SCALE)
                m_InitialObjectValue = obj.scale;
            else if (currentTool == Tool::ROTATE)
                m_InitialObjectValue = obj.rotation;
        }

        Color drawColor = hovered ? YELLOW : color;
        DrawCylinderEx(obj.position, endPos, lineThickness, lineThickness, 8, drawColor);

        if (currentTool == Tool::MOVE)
        {
            Vector3 coneBase = Vector3Subtract(endPos, Vector3Scale(direction, 0.4f));
            DrawCylinderEx(coneBase, endPos, 0.12f, 0.0f, 12, drawColor);
        }
        else if (currentTool == Tool::SCALE)
        {
            DrawCube(endPos, 0.25f, 0.25f, 0.25f, drawColor);
        }
        else
        {
            DrawSphere(endPos, handleRadius, drawColor);
        }
    };

    auto drawPlaneHandle = [&](GizmoAxis axis, Vector3 d1, Vector3 d2, Color color)
    {
        float pSize = 0.5f;
        Vector3 p1 = Vector3Add(obj.position, Vector3Scale(d1, pSize));
        Vector3 p2 =
            Vector3Add(obj.position, Vector3Add(Vector3Scale(d1, pSize), Vector3Scale(d2, pSize)));
        Vector3 p3 = Vector3Add(obj.position, Vector3Scale(d2, pSize));

        Vector3 normal = Vector3Normalize(Vector3CrossProduct(d1, d2));
        RayCollision planeColl = MyGetRayCollisionPlane(ray, obj.position, normal);

        bool hovered = (m_DraggingAxis == axis);
        if (!hovered && planeColl.hit && planeColl.distance < 10.0f)
        {
            Vector3 hitPos = planeColl.point;
            Vector3 rel = Vector3Subtract(hitPos, obj.position);
            float proj1 = Vector3DotProduct(rel, d1);
            float proj2 = Vector3DotProduct(rel, d2);
            if (proj1 >= 0 && proj1 <= pSize && proj2 >= 0 && proj2 <= pSize)
                hovered = true;
        }

        if (hovered)
            m_GizmoHovered = true;

        if (hovered && ImGui::IsMouseClicked(0) && m_DraggingAxis == GizmoAxis::NONE)
        {
            m_DraggingAxis = axis;
            m_InitialMousePos = mousePos;
            m_InitialObjectValue = obj.position;
        }

        Color drawColor = hovered ? YELLOW : color;
        drawColor.a = 150;
        DrawLine3D(obj.position, p1, drawColor);
        DrawLine3D(p1, p2, drawColor);
        DrawLine3D(p2, p3, drawColor);
        DrawLine3D(p3, obj.position, drawColor);
        Color fillColor = drawColor;
        fillColor.a = 60;
        DrawTriangle3D(obj.position, p1, p2, fillColor);
        DrawTriangle3D(obj.position, p2, p3, fillColor);
    };

    if (currentTool != Tool::SELECT)
    {
        drawAxisHandle(GizmoAxis::X, {1, 0, 0}, RED);
        drawAxisHandle(GizmoAxis::Y, {0, 1, 0}, GREEN);
        drawAxisHandle(GizmoAxis::Z, {0, 0, 1}, BLUE);

        if (currentTool == Tool::MOVE)
        {
            drawPlaneHandle(GizmoAxis::XY, {1, 0, 0}, {0, 1, 0}, RED);
            drawPlaneHandle(GizmoAxis::YZ, {0, 1, 0}, {0, 0, 1}, GREEN);
            drawPlaneHandle(GizmoAxis::XZ, {1, 0, 0}, {0, 0, 1}, BLUE);
        }
    }

    return m_GizmoHovered || m_DraggingAxis != GizmoAxis::NONE;
}

void ViewportGizmo::HandleGizmoDrag(MapObjectData &obj, Tool tool, ImVec2 currentMouse,
                                    const Camera3D &camera)
{
    Vector2 mouseDelta = {currentMouse.x - m_InitialMousePos.x,
                          currentMouse.y - m_InitialMousePos.y};

    float delta = (mouseDelta.x + mouseDelta.y) * 0.1f;

    if (tool == Tool::MOVE)
    {
        Vector3 newPos = m_InitialObjectValue;
        if (m_DraggingAxis == GizmoAxis::X)
            newPos.x += delta;
        else if (m_DraggingAxis == GizmoAxis::Y)
            newPos.y += delta;
        else if (m_DraggingAxis == GizmoAxis::Z)
            newPos.z += delta;
        else if (m_DraggingAxis == GizmoAxis::XY)
        {
            newPos.x += mouseDelta.x * 0.1f;
            newPos.y -= mouseDelta.y * 0.1f;
        }
        else if (m_DraggingAxis == GizmoAxis::YZ)
        {
            newPos.y -= mouseDelta.y * 0.1f;
            newPos.z += mouseDelta.x * 0.1f;
        }
        else if (m_DraggingAxis == GizmoAxis::XZ)
        {
            newPos.x += mouseDelta.x * 0.1f;
            newPos.z += mouseDelta.y * 0.1f;
        }

        if (m_SnappingEnabled)
        {
            newPos.x = SnapValue(newPos.x, m_GridSize);
            newPos.y = SnapValue(newPos.y, m_GridSize);
            newPos.z = SnapValue(newPos.z, m_GridSize);
        }
        obj.position = newPos;
    }
    else if (tool == Tool::ROTATE)
    {
        float rotDelta = mouseDelta.x * 0.5f;
        if (m_SnappingEnabled)
            rotDelta = SnapValue(rotDelta, m_RotationStep);

        if (m_DraggingAxis == GizmoAxis::X)
            obj.rotation.x = m_InitialObjectValue.x + rotDelta;
        else if (m_DraggingAxis == GizmoAxis::Y)
            obj.rotation.y = m_InitialObjectValue.y + rotDelta;
        else if (m_DraggingAxis == GizmoAxis::Z)
            obj.rotation.z = m_InitialObjectValue.z + rotDelta;
    }
    else if (tool == Tool::SCALE)
    {
        float scaleDelta = 1.0f + delta * 0.5f;
        if (m_DraggingAxis == GizmoAxis::X)
            obj.scale.x = fmaxf(0.1f, m_InitialObjectValue.x * scaleDelta);
        else if (m_DraggingAxis == GizmoAxis::Y)
            obj.scale.y = fmaxf(0.1f, m_InitialObjectValue.y * scaleDelta);
        else if (m_DraggingAxis == GizmoAxis::Z)
            obj.scale.z = fmaxf(0.1f, m_InitialObjectValue.z * scaleDelta);

        if (m_SnappingEnabled)
        {
            obj.scale.x = SnapValue(obj.scale.x, 0.1f); // Fixed scale snap for now
            obj.scale.y = SnapValue(obj.scale.y, 0.1f);
            obj.scale.z = SnapValue(obj.scale.z, 0.1f);
        }
    }
}

void ViewportGizmo::DrawGizmo(const MapObjectData &obj, const Camera3D &camera, Tool tool)
{
    // Implementation is now inside RenderAndHandle to share Ray logic easily.
    // I will leave this empty or move logic here if I decide to pass Ray.
    // For Hazel-style, it's better to keep it clean.
}

} // namespace CHEngine
