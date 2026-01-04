#include "editor_gizmo.h"
#include "editor_layer.h"
#include "engine/scene/components.h"
#include "logic/undo/transform_command.h"
#include <raymath.h>
#include <rlgl.h>

namespace CH
{

// Helper: Ray-Plane collision
static RayCollision GetRayCollisionPlane(Ray ray, Vector3 planePos, Vector3 planeNormal)
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

bool EditorGizmo::RenderAndHandle(Scene *scene, const Camera3D &camera, Entity entity,
                                  GizmoType type, ImVec2 viewportSize, bool isHovered)
{
    if (!scene || !entity)
        return false;

    if (!entity.HasComponent<TransformComponent>())
        return false;

    auto &transform = entity.GetComponent<TransformComponent>();

    // Reset hover state
    m_GizmoHovered = false;

    // Get mouse position for ray casting
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 viewportPos = ImGui::GetCursorScreenPos();

    // Convert to viewport-local coordinates
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
            // Drag finished - Push Undo Command
            EditorLayer::GetCommandHistory().PushCommand(
                std::make_unique<TransformCommand>(entity, m_OldTransform, transform));

            m_DraggingAxis = GizmoAxis::NONE;
        }
        else
        {
            Vector2 mouseDelta = {mousePos.x - m_InitialMousePos.x,
                                  mousePos.y - m_InitialMousePos.y};
            float delta = (mouseDelta.x + mouseDelta.y) * 0.1f;

            if (type == GizmoType::TRANSLATE)
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
                transform.Translation = newPos;
            }
            else if (type == GizmoType::ROTATE)
            {
                float rotDelta = mouseDelta.x * 0.5f * DEG2RAD;
                if (m_SnappingEnabled)
                    rotDelta = SnapValue(rotDelta, m_RotationStep * DEG2RAD);

                if (m_DraggingAxis == GizmoAxis::X)
                    transform.Rotation.x = m_InitialObjectValue.x + rotDelta;
                else if (m_DraggingAxis == GizmoAxis::Y)
                    transform.Rotation.y = m_InitialObjectValue.y + rotDelta;
                else if (m_DraggingAxis == GizmoAxis::Z)
                    transform.Rotation.z = m_InitialObjectValue.z + rotDelta;
            }
            else if (type == GizmoType::SCALE)
            {
                float scaleDelta = 1.0f + delta * 0.5f;
                if (m_DraggingAxis == GizmoAxis::X)
                    transform.Scale.x = fmaxf(0.1f, m_InitialObjectValue.x * scaleDelta);
                else if (m_DraggingAxis == GizmoAxis::Y)
                    transform.Scale.y = fmaxf(0.1f, m_InitialObjectValue.y * scaleDelta);
                else if (m_DraggingAxis == GizmoAxis::Z)
                    transform.Scale.z = fmaxf(0.1f, m_InitialObjectValue.z * scaleDelta);

                if (m_SnappingEnabled)
                {
                    transform.Scale.x = SnapValue(transform.Scale.x, 0.1f);
                    transform.Scale.y = SnapValue(transform.Scale.y, 0.1f);
                    transform.Scale.z = SnapValue(transform.Scale.z, 0.1f);
                }
            }
        }
    }

    // Draw gizmo handles
    float gizmoSize = 2.0f;
    float lineThickness = 0.05f;

    auto drawAxisHandle = [&](GizmoAxis axis, Vector3 direction, Color color)
    {
        Vector3 endPos = Vector3Add(transform.Translation, Vector3Scale(direction, gizmoSize));

        // Interaction logic (bounding box for picking)
        BoundingBox handleBox = {Vector3Subtract(endPos, {0.3f, 0.3f, 0.3f}),
                                 Vector3Add(endPos, {0.3f, 0.3f, 0.3f})};
        RayCollision handleColl = GetRayCollisionBox(ray, handleBox);

        RayCollision lineColl = GetRayCollisionBox(
            ray, {Vector3Subtract(Vector3Min(transform.Translation, endPos), {0.1f, 0.1f, 0.1f}),
                  Vector3Add(Vector3Max(transform.Translation, endPos), {0.1f, 0.1f, 0.1f})});

        bool hovered = (m_DraggingAxis == axis) || handleColl.hit ||
                       (lineColl.hit && lineColl.distance < 10.0f);

        if (hovered)
            m_GizmoHovered = true;

        // Start dragging
        if (hovered && ImGui::IsMouseClicked(0) && m_DraggingAxis == GizmoAxis::NONE && isHovered)
        {
            m_DraggingAxis = axis;
            m_InitialMousePos = mousePos;
            m_OldTransform = transform;

            if (type == GizmoType::TRANSLATE)
                m_InitialObjectValue = transform.Translation;
            else if (type == GizmoType::SCALE)
                m_InitialObjectValue = transform.Scale;
            else if (type == GizmoType::ROTATE)
                m_InitialObjectValue = transform.Rotation;
        }

        // Draw visual
        Color drawColor = hovered ? YELLOW : color;
        DrawCylinderEx(transform.Translation, endPos, lineThickness, lineThickness, 8, drawColor);

        if (type == GizmoType::TRANSLATE)
        {
            // Arrow cone
            Vector3 coneBase = Vector3Subtract(endPos, Vector3Scale(direction, 0.4f));
            DrawCylinderEx(coneBase, endPos, 0.12f, 0.0f, 12, drawColor);
        }
        else if (type == GizmoType::SCALE)
        {
            // Cube
            DrawCube(endPos, 0.25f, 0.25f, 0.25f, drawColor);
        }
    };

    // Render axes
    if (type != GizmoType::SELECT)
    {
        drawAxisHandle(GizmoAxis::X, {1, 0, 0}, RED);
        drawAxisHandle(GizmoAxis::Y, {0, 1, 0}, GREEN);
        drawAxisHandle(GizmoAxis::Z, {0, 0, 1}, BLUE);
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
