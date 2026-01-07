#include "editor_gizmo.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "logic/undo/transform_command.h"
#include <raymath.h>
#include <rlgl.h>

namespace CH
{

static RayCollision GetRayCollisionPlane(Ray ray, Vector3 planePos, Vector3 planeNormal)
{
    RayCollision collision = {0};

    float denom = Vector3DotProduct(planeNormal, ray.direction);
    if (fabsf(denom) > 0.0001f)
    {
        float t = Vector3DotProduct(Vector3Subtract(planePos, ray.position), planeNormal) / denom;

        if (t >= 0.0f)
        {
            collision.hit = true;
            collision.distance = t;
            collision.point = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
            collision.normal = planeNormal;
        }
    }
    return collision;
}

bool EditorGizmo::RenderAndHandle(Scene *scene, const Camera3D &camera, Entity entity,
                                  GizmoType type, ImVec2 viewportSize, bool isHovered)
{
    if (!scene || !entity || !entity.HasComponent<TransformComponent>())
        return false;

    auto &transform = entity.GetComponent<TransformComponent>();
    m_GizmoHovered = false;

    // ------------------------------------------------------------
    // Mouse → Ray
    // ------------------------------------------------------------
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    Vector2 localMouse = {mousePos.x - viewportPos.x, mousePos.y - viewportPos.y};

    float ndcX = (2.0f * localMouse.x) / viewportSize.x - 1.0f;
    float ndcY = 1.0f - (2.0f * localMouse.y) / viewportSize.y;

    float aspect = viewportSize.x / viewportSize.y;
    float tanHalfFovy = tanf(camera.fovy * 0.5f * DEG2RAD);

    Vector3 rayDirView = {ndcX * aspect * tanHalfFovy, ndcY * tanHalfFovy, 1.0f};

    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    Vector3 up = Vector3CrossProduct(right, forward);

    Vector3 rayDirWorld =
        Vector3Normalize({rayDirView.x * right.x + rayDirView.y * up.x + rayDirView.z * forward.x,
                          rayDirView.x * right.y + rayDirView.y * up.y + rayDirView.z * forward.y,
                          rayDirView.x * right.z + rayDirView.y * up.z + rayDirView.z * forward.z});

    Ray ray = {camera.position, rayDirWorld};

    // ------------------------------------------------------------
    // Drag update
    // ------------------------------------------------------------
    if (m_DraggingAxis != GizmoAxis::NONE)
    {
        if (!ImGui::IsMouseDown(0))
        {
            EditorLayer::GetCommandHistory().PushCommand(
                std::make_unique<TransformCommand>(entity, m_OldTransform, transform));

            m_DraggingAxis = GizmoAxis::NONE;
        }
        else
        {
            auto hit = GetRayCollisionPlane(ray, m_DragPlanePos, m_DragPlaneNormal);
            if (hit.hit)
            {
                Vector3 delta = Vector3Subtract(hit.point, m_DragStartHit);

                if (m_DraggingAxis == GizmoAxis::X)
                    delta = {delta.x, 0, 0};
                if (m_DraggingAxis == GizmoAxis::Y)
                    delta = {0, delta.y, 0};
                if (m_DraggingAxis == GizmoAxis::Z)
                    delta = {0, 0, delta.z};

                Vector3 newPos = Vector3Add(m_DragStartValue, delta);

                if (m_SnappingEnabled)
                {
                    newPos.x = SnapValue(newPos.x, m_GridSize);
                    newPos.y = SnapValue(newPos.y, m_GridSize);
                    newPos.z = SnapValue(newPos.z, m_GridSize);
                }

                transform.Translation = newPos;
            }
        }
    }

    // ------------------------------------------------------------
    // Draw gizmo axes
    // ------------------------------------------------------------
    float gizmoSize = 5.0f;
    float thickness = 0.1f;

    auto drawAxis = [&](GizmoAxis axis, Vector3 dir, Color color)
    {
        Vector3 start = transform.Translation;
        Vector3 end = Vector3Add(start, Vector3Scale(dir, gizmoSize));

        BoundingBox box = {Vector3Subtract(end, {0.3f, 0.3f, 0.3f}),
                           Vector3Add(end, {0.3f, 0.3f, 0.3f})};

        RayCollision hit = GetRayCollisionBox(ray, box);

        // Restore shaft collision
        Vector3 minV = Vector3Min(start, end);
        Vector3 maxV = Vector3Max(start, end);
        BoundingBox lineBox = {Vector3Subtract(minV, {0.1f, 0.1f, 0.1f}),
                               Vector3Add(maxV, {0.1f, 0.1f, 0.1f})};
        RayCollision lineHit = GetRayCollisionBox(ray, lineBox);

        bool hovered = hit.hit || (lineHit.hit && lineHit.distance < 10.0f);

        if (hovered)
            m_GizmoHovered = true;

        if (hovered && ImGui::IsMouseClicked(0) && m_DraggingAxis == GizmoAxis::NONE && isHovered)
        {
            m_DraggingAxis = axis;
            m_OldTransform = transform;

            // Площина руху
            if (axis == GizmoAxis::X)
                m_DragPlaneNormal = {0, 1, 0}; // YZ
            else if (axis == GizmoAxis::Y)
                m_DragPlaneNormal = {1, 0, 0}; // XZ
            else if (axis == GizmoAxis::Z)
                m_DragPlaneNormal = {0, 1, 0}; // XY

            m_DragPlanePos = transform.Translation;
            m_DragStartValue = transform.Translation;

            auto pHit = GetRayCollisionPlane(ray, m_DragPlanePos, m_DragPlaneNormal);
            if (pHit.hit)
                m_DragStartHit = pHit.point;
        }

        Color drawColor = (hovered || m_DraggingAxis == axis) ? YELLOW : color;
        DrawCylinderEx(start, end, thickness, thickness, 8, drawColor);

        Vector3 coneBase = Vector3Subtract(end, Vector3Scale(dir, 0.4f));
        DrawCylinderEx(coneBase, end, 0.15f, 0.0f, 12, drawColor);
    };

    if (type == GizmoType::TRANSLATE)
    {
        drawAxis(GizmoAxis::X, {1, 0, 0}, RED);
        drawAxis(GizmoAxis::Y, {0, 1, 0}, GREEN);
        drawAxis(GizmoAxis::Z, {0, 0, 1}, BLUE);
    }

    return m_GizmoHovered || m_DraggingAxis != GizmoAxis::NONE;
}

float EditorGizmo::SnapValue(float value, float step)
{
    if (step <= 0.0f)
        return value;
    return std::roundf(value / step) * step;
}

} // namespace CH
