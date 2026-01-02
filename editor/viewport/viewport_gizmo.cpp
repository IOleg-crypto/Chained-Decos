#include "viewport_gizmo.h"
#include "editor/logic/undo/command_history.h"
#include "editor/logic/undo/transform_command.h"
#include "editor/panels/viewport_panel.h"
#include "scene/core/scene.h"
#include "scene/ecs/components/transform_component.h"
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

bool ViewportGizmo::RenderAndHandle(std::shared_ptr<Scene> scene, const Camera3D &camera,
                                    entt::entity entity, Tool currentTool, ImVec2 viewportSize,
                                    bool isHovered, CommandHistory *history)
{
    if (!scene || entity == entt::null)
        return false;

    if (!scene->GetRegistry().all_of<TransformComponent>(entity))
        return false;

    auto &transform = scene->GetRegistry().get<TransformComponent>(entity);

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
                          rayDir.x * right.y + rayDir.y * up.y + rayDir.z * forward.z});

    Ray ray = {camera.position, worldRayDir};

    // Handle dragging
    if (m_DraggingAxis != GizmoAxis::NONE)
    {
        if (!ImGui::IsMouseDown(0))
        {
            // Drag finished
            // TODO: Push Undo Command for TransformComponent
            m_DraggingAxis = GizmoAxis::NONE;
        }
        else
        {
            Vector2 mouseDelta = {mousePos.x - m_InitialMousePos.x,
                                  mousePos.y - m_InitialMousePos.y};
            float delta = (mouseDelta.x + mouseDelta.y) * 0.1f;

            if (currentTool == Tool::MOVE)
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
                transform.position = newPos;
            }
            else if (currentTool == Tool::ROTATE)
            {
                float rotDelta = mouseDelta.x * 0.5f;
                if (m_SnappingEnabled)
                    rotDelta = SnapValue(rotDelta, m_RotationStep);

                if (m_DraggingAxis == GizmoAxis::X)
                    transform.rotation.x = m_InitialObjectValue.x + rotDelta;
                else if (m_DraggingAxis == GizmoAxis::Y)
                    transform.rotation.y = m_InitialObjectValue.y + rotDelta;
                else if (m_DraggingAxis == GizmoAxis::Z)
                    transform.rotation.z = m_InitialObjectValue.z + rotDelta;
            }
            else if (currentTool == Tool::SCALE)
            {
                float scaleDelta = 1.0f + delta * 0.5f;
                if (m_DraggingAxis == GizmoAxis::X)
                    transform.scale.x = fmaxf(0.1f, m_InitialObjectValue.x * scaleDelta);
                else if (m_DraggingAxis == GizmoAxis::Y)
                    transform.scale.y = fmaxf(0.1f, m_InitialObjectValue.y * scaleDelta);
                else if (m_DraggingAxis == GizmoAxis::Z)
                    transform.scale.z = fmaxf(0.1f, m_InitialObjectValue.z * scaleDelta);

                if (m_SnappingEnabled)
                {
                    transform.scale.x = SnapValue(transform.scale.x, 0.1f);
                    transform.scale.y = SnapValue(transform.scale.y, 0.1f);
                    transform.scale.z = SnapValue(transform.scale.z, 0.1f);
                }
            }
        }
    }

    // Draw handles (and check for picking)
    float gizmoSize = 2.0f;
    float handleRadius = 0.2f;
    float lineThickness = 0.05f;

    auto drawAxisHandle = [&](GizmoAxis axis, Vector3 direction, Color color)
    {
        Vector3 endPos = Vector3Add(transform.position, Vector3Scale(direction, gizmoSize));

        // Interaction logic
        BoundingBox handleBox = {Vector3Subtract(endPos, {0.3f, 0.3f, 0.3f}),
                                 Vector3Add(endPos, {0.3f, 0.3f, 0.3f})};
        RayCollision handleColl = GetRayCollisionBox(ray, handleBox);

        RayCollision lineColl = GetRayCollisionBox(
            ray, {Vector3Subtract(Vector3Min(transform.position, endPos), {0.1f, 0.1f, 0.1f}),
                  Vector3Add(Vector3Max(transform.position, endPos), {0.1f, 0.1f, 0.1f})});

        bool hovered = (m_DraggingAxis == axis) || handleColl.hit ||
                       (lineColl.hit && lineColl.distance < 10.0f);

        if (hovered)
            m_GizmoHovered = true;

        if (hovered && ImGui::IsMouseClicked(0) && m_DraggingAxis == GizmoAxis::NONE)
        {
            m_DraggingAxis = axis;
            m_InitialMousePos = mousePos;

            if (currentTool == Tool::MOVE)
                m_InitialObjectValue = transform.position;
            else if (currentTool == Tool::SCALE)
                m_InitialObjectValue = transform.scale;
            else if (currentTool == Tool::ROTATE)
                m_InitialObjectValue = transform.rotation;
        }

        Color drawColor = hovered ? YELLOW : color;
        DrawCylinderEx(transform.position, endPos, lineThickness, lineThickness, 8, drawColor);

        if (currentTool == Tool::MOVE)
        {
            Vector3 coneBase = Vector3Subtract(endPos, Vector3Scale(direction, 0.4f));
            DrawCylinderEx(coneBase, endPos, 0.12f, 0.0f, 12, drawColor);
        }
        else if (currentTool == Tool::SCALE)
        {
            DrawCube(endPos, 0.25f, 0.25f, 0.25f, drawColor);
        }
    };

    if (currentTool != Tool::SELECT)
    {
        drawAxisHandle(GizmoAxis::X, {1, 0, 0}, RED);
        drawAxisHandle(GizmoAxis::Y, {0, 1, 0}, GREEN);
        drawAxisHandle(GizmoAxis::Z, {0, 0, 1}, BLUE);
    }

    return m_GizmoHovered || m_DraggingAxis != GizmoAxis::NONE;
}

float ViewportGizmo::SnapValue(float value, float step)
{
    if (step <= 0.0f)
        return value;
    return std::roundf(value / step) * step;
}
} // namespace CHEngine
