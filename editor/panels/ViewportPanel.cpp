#include "ViewportPanel.h"
#include "../viewport/ViewportPicking.h"

#include "scene/resources/map/MapRenderer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <raymath.h>
#include <rlImGui.h>
#include <rlgl.h>

namespace CHEngine
{
// Internal helper for plane collision if not in raylib
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

ViewportPanel::~ViewportPanel()
{
    if (m_ViewportTexture.id != 0)
        UnloadRenderTexture(m_ViewportTexture);
}

void ViewportPanel::OnImGuiRender(const std::shared_ptr<GameScene> &scene,
                                  const std::shared_ptr<CameraController> &cameraController,
                                  int selectedObjectIndex, Tool currentTool,
                                  const std::function<void(int)> &onSelect)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Viewport", nullptr, windowFlags);

    m_Focused = ImGui::IsWindowFocused();
    m_Hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
                                       ImGuiHoveredFlags_ChildWindows);

    ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    if (m_Width != (uint32_t)viewportPanelSize.x || m_Height != (uint32_t)viewportPanelSize.y)
    {
        Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
    }

    if (m_ViewportTexture.id != 0)
    {
        // 1. Render Scene to Texture
        BeginTextureMode(m_ViewportTexture);
        ClearBackground(DARKGRAY);

        if (scene && cameraController)
        {
            m_GizmoHovered = false; // Reset every frame
            MapRenderer renderer;
            // Enter 3D Mode for ALL editor 3D drawing (Objects + Helpers)
            BeginMode3D(cameraController->GetCamera());

            // Ensure depth test is ON for Solid objects
            rlEnableDepthTest();
            rlEnableDepthMask();

            // 1. Draw Map Content (Solid Objects)
            renderer.DrawMapContent(*scene, cameraController->GetCamera());

            // 2. Editor Helpers initialization (Draw moved below)
            if (!m_GridInitialized)
            {
                m_Grid.Init();
                m_GridInitialized = true;
            }

            // Calculate picking ray INSIDE the texture mode for accuracy
            // Calculate picking ray manually to avoid reliance on Raylib's window size
            Vector2 viewportMouse = GetViewportMousePosition();
            float mouseX = (viewportMouse.x / (float)m_Width) * 2.0f - 1.0f;
            float mouseY = 1.0f - (viewportMouse.y / (float)m_Height) * 2.0f;

            Matrix view = GetCameraMatrix(cameraController->GetCamera());
            float aspect = (float)m_Width / (float)m_Height;
            Matrix projection = MatrixPerspective(cameraController->GetCamera().fovy * DEG2RAD,
                                                  aspect, 0.01f, 1000.0f);
            Matrix invProjView = MatrixInvert(MatrixMultiply(view, projection));

            Vector3 nearPoint = Vector3Transform({mouseX, mouseY, -1.0f}, invProjView);
            Vector3 farPoint = Vector3Transform({mouseX, mouseY, 1.0f}, invProjView);

            Ray pickingRay;
            pickingRay.position = cameraController->GetCamera().position;
            pickingRay.direction = Vector3Normalize(Vector3Subtract(farPoint, nearPoint));

            // 3. Object Picking (If hovered, clicked, and NOT clicking on a gizmo)
            if (m_Hovered && ImGui::IsMouseClicked(0) && m_DraggingAxis == GizmoAxis::NONE &&
                !m_GizmoHovered)
            {
                int hitIndex = -1;
                float closestDist = FLT_MAX;
                auto &objects = scene->GetMapObjects();

                for (int i = 0; i < (int)objects.size(); i++)
                {
                    const auto &obj = objects[i];

                    // Transform ray to local space to handle rotation
                    Matrix worldMat =
                        MatrixTranslate(obj.position.x, obj.position.y, obj.position.z);
                    worldMat = MatrixMultiply(MatrixRotateXYZ(Vector3Scale(obj.rotation, DEG2RAD)),
                                              worldMat);
                    Matrix invWorldMat = MatrixInvert(worldMat);

                    Ray localRay;
                    localRay.position = Vector3Transform(pickingRay.position, invWorldMat);
                    localRay.direction = Vector3Normalize(Vector3Subtract(
                        Vector3Transform(Vector3Add(pickingRay.position, pickingRay.direction),
                                         invWorldMat),
                        localRay.position));

                    BoundingBox localBox;
                    if (obj.type == MapObjectType::CUBE)
                    {
                        localBox = {Vector3Scale(obj.scale, -0.5f), Vector3Scale(obj.scale, 0.5f)};
                    }
                    else if (obj.type == MapObjectType::SPHERE)
                    {
                        localBox = {{-obj.radius, -obj.radius, -obj.radius},
                                    {obj.radius, obj.radius, obj.radius}};
                    }
                    else if (obj.type == MapObjectType::CYLINDER)
                    {
                        localBox = {{-obj.radius, -obj.height / 2, -obj.radius},
                                    {obj.radius, obj.height / 2, obj.radius}};
                    }
                    else if (obj.type == MapObjectType::PLANE)
                    {
                        localBox = {{-obj.size.x / 2, -0.05f, -obj.size.y / 2},
                                    {obj.size.x / 2, 0.05f, obj.size.y / 2}};
                    }
                    else
                    {
                        localBox = {{-0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}};
                    }

                    RayCollision collision = GetRayCollisionBox(localRay, localBox);
                    if (collision.hit && collision.distance < closestDist)
                    {
                        closestDist = collision.distance;
                        hitIndex = i;
                    }
                }

                if (hitIndex != -1)
                    onSelect(hitIndex);
            }

            // 4. Draw Gizmo for selected object
            if (selectedObjectIndex >= 0 &&
                selectedObjectIndex < (int)scene->GetMapObjects().size())
            {
                auto &obj = scene->GetMapObjectsMutable()[selectedObjectIndex];
                Ray ray = pickingRay;

                // 2. Handle Dragging
                if (m_DraggingAxis != GizmoAxis::NONE)
                {
                    if (!ImGui::IsMouseDown(0))
                    {
                        m_DraggingAxis = GizmoAxis::NONE;
                    }
                    else
                    {
                        Vector2 currentMouse = GetViewportMousePosition();
                        Vector2 mouseDelta = Vector2Subtract(currentMouse, m_InitialMousePos);

                        if (m_DraggingAxis == GizmoAxis::X || m_DraggingAxis == GizmoAxis::Y ||
                            m_DraggingAxis == GizmoAxis::Z)
                        {
                            Vector3 axisDir = {0};
                            if (m_DraggingAxis == GizmoAxis::X)
                                axisDir = {1, 0, 0};
                            else if (m_DraggingAxis == GizmoAxis::Y)
                                axisDir = {0, 1, 0};
                            else if (m_DraggingAxis == GizmoAxis::Z)
                                axisDir = {0, 0, 1};

                            Vector2 screenAxisDir = Vector2Subtract(
                                GetViewportWorldToScreen(Vector3Add(obj.position, axisDir),
                                                         cameraController->GetCamera()),
                                GetViewportWorldToScreen(obj.position,
                                                         cameraController->GetCamera()));
                            screenAxisDir = Vector2Normalize(screenAxisDir);

                            float delta =
                                (mouseDelta.x * screenAxisDir.x + mouseDelta.y * screenAxisDir.y) *
                                0.1f;

                            if (currentTool == Tool::MOVE)
                            {
                                if (m_DraggingAxis == GizmoAxis::X)
                                    obj.position.x = m_InitialObjectValue.x + delta;
                                else if (m_DraggingAxis == GizmoAxis::Y)
                                    obj.position.y = m_InitialObjectValue.y + delta;
                                else if (m_DraggingAxis == GizmoAxis::Z)
                                    obj.position.z = m_InitialObjectValue.z + delta;
                            }
                            else if (currentTool == Tool::ROTATE)
                            {
                                float rotDelta = (currentMouse.x - m_InitialMousePos.x) * 0.5f;
                                if (m_DraggingAxis == GizmoAxis::X)
                                    obj.rotation.x = m_InitialObjectValue.x + rotDelta;
                                else if (m_DraggingAxis == GizmoAxis::Y)
                                    obj.rotation.y = m_InitialObjectValue.y + rotDelta;
                                else if (m_DraggingAxis == GizmoAxis::Z)
                                    obj.rotation.z = m_InitialObjectValue.z + rotDelta;
                            }
                            else if (currentTool == Tool::SCALE)
                            {
                                float scaleDelta = 1.0f + delta * 0.5f;
                                if (m_DraggingAxis == GizmoAxis::X)
                                    obj.scale.x = fmaxf(0.1f, m_InitialObjectValue.x * scaleDelta);
                                else if (m_DraggingAxis == GizmoAxis::Y)
                                    obj.scale.y = fmaxf(0.1f, m_InitialObjectValue.y * scaleDelta);
                                else if (m_DraggingAxis == GizmoAxis::Z)
                                    obj.scale.z = fmaxf(0.1f, m_InitialObjectValue.z * scaleDelta);
                            }
                        }
                    }
                }

                // 3. Draw Handles
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

                    // Line collision (slightly thicker for easier picking)
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
                        m_InitialMousePos = GetViewportMousePosition();
                        if (currentTool == Tool::MOVE)
                            m_InitialObjectValue = obj.position;
                        else if (currentTool == Tool::SCALE)
                            m_InitialObjectValue = obj.scale;
                        else if (currentTool == Tool::ROTATE)
                            m_InitialObjectValue = obj.rotation;
                    }

                    Color drawColor = hovered ? YELLOW : color;

                    // Draw Thick Axis Line
                    DrawCylinderEx(obj.position, endPos, lineThickness, lineThickness, 8,
                                   drawColor);

                    // Draw Tool-Specific Head
                    if (currentTool == Tool::MOVE)
                    {
                        // Draw Cone (Arrow)
                        Vector3 coneBase = Vector3Subtract(endPos, Vector3Scale(direction, 0.4f));
                        DrawCylinderEx(coneBase, endPos, 0.12f, 0.0f, 12, drawColor);
                    }
                    else if (currentTool == Tool::SCALE)
                    {
                        // Draw Cube
                        DrawCube(endPos, 0.25f, 0.25f, 0.25f, drawColor);
                    }
                    else
                    {
                        // Fallback/Rotate
                        DrawSphere(endPos, handleRadius, drawColor);
                    }
                };

                auto drawPlaneHandle = [&](GizmoAxis axis, Vector3 d1, Vector3 d2, Color color)
                {
                    float pSize = 0.5f;
                    Vector3 p1 = Vector3Add(obj.position, Vector3Scale(d1, pSize));
                    Vector3 p2 = Vector3Add(
                        obj.position, Vector3Add(Vector3Scale(d1, pSize), Vector3Scale(d2, pSize)));
                    Vector3 p3 = Vector3Add(obj.position, Vector3Scale(d2, pSize));

                    Vector3 normal = Vector3Normalize(Vector3CrossProduct(d1, d2));
                    RayCollision planeColl = MyGetRayCollisionPlane(ray, obj.position, normal);

                    bool hovered = (m_DraggingAxis == axis);
                    if (!hovered && planeColl.hit && planeColl.distance < 5.0f)
                    {
                        Vector3 hitPos = planeColl.point;
                        Vector3 rel = Vector3Subtract(hitPos, obj.position);
                        float proj1 = Vector3DotProduct(rel, d1);
                        float proj2 = Vector3DotProduct(rel, d2);
                        if (proj1 >= 0 && proj1 <= pSize && proj2 >= 0 && proj2 <= pSize)
                            hovered = true;
                    }

                    if (hovered && ImGui::IsMouseClicked(0) && m_DraggingAxis == GizmoAxis::NONE)
                    {
                        m_DraggingAxis = axis;
                        m_InitialMousePos = GetViewportMousePosition();
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

                // Draw selection highlight for all types (Models and Primitives)
                renderer.RenderMapObject(obj, scene->GetMapModels(), cameraController->GetCamera(),
                                         true, true);
            }

            // Draw Infinite Grid last (with depth testing)
            m_Grid.Draw(cameraController->GetCamera(), m_Width, m_Height);

            EndMode3D();

            // 5. Draw 2D Labels AFTER EndMode3D
            if (selectedObjectIndex >= 0 &&
                selectedObjectIndex < (int)scene->GetMapObjects().size())
            {
                auto &obj = scene->GetMapObjects()[selectedObjectIndex];
                if (currentTool != Tool::SELECT)
                {
                    float gizmoSize = 2.0f;
                    auto drawLabel = [&](Vector3 dir, const char *label, Color color)
                    {
                        Vector3 endPos = Vector3Add(obj.position, Vector3Scale(dir, gizmoSize));
                        Vector2 screenPos =
                            GetViewportWorldToScreen(endPos, cameraController->GetCamera());
                        DrawText(label, (int)screenPos.x + 5, (int)screenPos.y - 10, 20, color);
                    };
                    drawLabel({1, 0, 0}, "X", RED);
                    drawLabel({0, 1, 0}, "Y", GREEN);
                    drawLabel({0, 0, 1}, "Z", BLUE);
                }
            }
        }

        EndTextureMode();
        rlImGuiImageRenderTextureFit(&m_ViewportTexture, true);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::Resize(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return;
    if (m_ViewportTexture.id != 0)
        UnloadRenderTexture(m_ViewportTexture);
    m_Width = width;
    m_Height = height;
    m_ViewportTexture = LoadRenderTexture(m_Width, m_Height);
}

Vector2 ViewportPanel::GetViewportMousePosition() const
{
    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentPos = ImGui::GetWindowContentRegionMin();

    return {mousePos.x - (windowPos.x + contentPos.x), mousePos.y - (windowPos.y + contentPos.y)};
}

Vector2 ViewportPanel::GetViewportWorldToScreen(Vector3 worldPos, Camera3D camera) const
{
    // Calculate normalized device coordinates manually
    Matrix view = GetCameraMatrix(camera);
    float aspect = (float)m_Width / (float)m_Height;
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD, aspect, 0.01f, 1000.0f);

    // Transform world position to clip space
    Vector3 clipPos = Vector3Transform(worldPos, MatrixMultiply(view, projection));

    // Perspective division to get NDC (Normalized Device Coordinates)
    // Note: Vector1/Raylib math doesn't handle W division in Vector3Transform for Matrix4x4
    // automatically in a way that gives NDC. We need the 4th component.

    // Let's use internal raylib math if possible, but correctly.
    // Actually, Raylib's GetWorldToScreen can be used IF we compensate for its internal use of
    // Screen size.

    Vector2 raylibScreenPos = GetWorldToScreen(worldPos, camera);

    // Map from Raylib's window coordinates to our viewport texture coordinates
    float screenWidth = (float)GetScreenWidth();
    float screenHeight = (float)GetScreenHeight();

    Vector2 viewportPos;
    viewportPos.x = (raylibScreenPos.x / screenWidth) * (float)m_Width;
    viewportPos.y = (raylibScreenPos.y / screenHeight) * (float)m_Height;

    return viewportPos;
}
} // namespace CHEngine
