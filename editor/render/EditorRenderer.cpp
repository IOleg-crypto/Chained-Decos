#include "editor/render/EditorRenderer.h"
#include "components/rendering/utils/RenderUtils.h"
#include "editor/IEditor.h"
#include "editor/tool/IToolManager.h"
#include "scene/resources/map/renderer/MapRenderer.h"
#include <filesystem>
#include <raylib.h>
#include <raymath.h>
#include <unordered_map>

EditorRenderer::EditorRenderer(IEditor *editor, IToolManager *toolManager)
    : m_editor(editor), m_toolManager(toolManager)
{
}

void EditorRenderer::RenderObject(const MapObjectData &data, bool isSelected)
{
    // Handle spawn zone rendering separately
    if (data.type == MapObjectType::SPAWN_ZONE)
    {
        return;
    }

    // Get loaded models from Editor's ModelLoader
    std::unordered_map<std::string, Model> loadedModels;
    if (m_editor && data.type == MapObjectType::MODEL && !data.modelName.empty())
    {
        // Try exact match first
        auto modelOpt = m_editor->GetGameMap().GetMapModels().find(data.modelName);

        if (modelOpt != m_editor->GetGameMap().GetMapModels().end())
        {
            loadedModels[data.modelName] = modelOpt->second;
        }
    }

    Camera3D camera = m_editor ? m_editor->GetCameraController().GetCamera() : Camera3D{};
    MapRenderer renderer;
    renderer.RenderMapObject(data, loadedModels, camera, true);

    // Additional editor-specific rendering: selection wireframe and gizmo
    if (isSelected)
    {
        // Render gizmo for MOVE and SCALE tools
        RenderGizmo(data);

        // Render selection wireframe
        if (data.type == MapObjectType::MODEL)
        {
            auto it = loadedModels.find(data.modelName);
            if (it != loadedModels.end())
            {
                // Draw wireframe for selected models
                DrawModelWires(it->second, {0, 0, 0}, 1.0f, YELLOW);
            }
        }
        else
        {
            RenderSelectionWireframe(data);
        }
    }
}

void EditorRenderer::RenderGizmo(const MapObjectData &data)
{
    // Only show gizmo for MOVE and SCALE tools
    if (!m_toolManager || !m_editor)
        return;

    Tool activeTool = m_toolManager->GetActiveTool();
    if (activeTool != MOVE && activeTool != SCALE)
        return;

    Vector3 pos = data.position;
    float gizmoLength = 2.0f;
    float gizmoThickness = 0.1f;

    // Get camera to determine gizmo scale
    Camera3D camera = m_editor->GetCameraController().GetCamera();
    Vector3 toCamera = Vector3Subtract(camera.position, pos);
    float distance = Vector3Length(toCamera);
    float scale = distance * 0.1f; // Scale gizmo based on distance from camera
    if (scale < 0.5f)
        scale = 0.5f;
    if (scale > 2.0f)
        scale = 2.0f;

    float arrowLength = gizmoLength * scale;
    float arrowRadius = gizmoThickness * scale;

    // Draw X axis (red) - right
    DrawLine3D(pos, Vector3Add(pos, {arrowLength, 0, 0}), RED);
    DrawCylinderEx(Vector3Add(pos, {arrowLength * 0.7f, 0, 0}),
                   Vector3Add(pos, {arrowLength, 0, 0}), arrowRadius * 0.5f, arrowRadius, 8, RED);

    // Draw Y axis (green) - up
    DrawLine3D(pos, Vector3Add(pos, {0, arrowLength, 0}), GREEN);
    DrawCylinderEx(Vector3Add(pos, {0, arrowLength * 0.7f, 0}),
                   Vector3Add(pos, {0, arrowLength, 0}), arrowRadius * 0.5f, arrowRadius, 8, GREEN);

    // Draw Z axis (blue) - forward
    DrawLine3D(pos, Vector3Add(pos, {0, 0, arrowLength}), BLUE);
    DrawCylinderEx(Vector3Add(pos, {0, 0, arrowLength * 0.7f}),
                   Vector3Add(pos, {0, 0, arrowLength}), arrowRadius * 0.5f, arrowRadius, 8, BLUE);

    // Draw center sphere
    DrawSphere(pos, arrowRadius * 1.5f, YELLOW);
}

void EditorRenderer::RenderSpawnZoneWithTexture(Texture2D texture, const Vector3 &position,
                                                float size, Color color, bool textureLoaded)
{
    if (!textureLoaded)
    {
        // Fallback to simple cube if texture not loaded
        DrawCube(position, size, size, size, color);
        DrawCubeWires(position, size, size, size, WHITE);
        return;
    }

    // Use shared RenderUtils function to draw textured cube
    RenderUtils::DrawCubeTexture(texture, position, size, size, size, color);

    // Draw wireframe for better visibility
    DrawCubeWires(position, size, size, size, WHITE);
}

void EditorRenderer::RenderSelectionWireframe(const MapObjectData &data)
{
    // Draw selection indicator for primitives
    Color selectionColor = YELLOW;
    selectionColor.a = 100; // Semi-transparent

    switch (data.type)
    {
    case MapObjectType::CUBE:
    {
        DrawCubeWires(data.position, data.scale.x, data.scale.y, data.scale.z, YELLOW);
        break;
    }
    case MapObjectType::SPHERE:
    {
        DrawSphereWires(data.position, data.radius, 16, 16, YELLOW);
        break;
    }
    case MapObjectType::CYLINDER:
    {
        DrawCylinderWires(data.position, data.radius, data.radius, data.height, 16, YELLOW);
        break;
    }
    case MapObjectType::PLANE:
    {
        // Draw plane selection indicator using lines
        Vector3 p1 = {data.position.x - data.size.x * 0.5f, data.position.y,
                      data.position.z - data.size.y * 0.5f};
        Vector3 p2 = {data.position.x + data.size.x * 0.5f, data.position.y,
                      data.position.z - data.size.y * 0.5f};
        Vector3 p3 = {data.position.x + data.size.x * 0.5f, data.position.y,
                      data.position.z + data.size.y * 0.5f};
        Vector3 p4 = {data.position.x - data.size.x * 0.5f, data.position.y,
                      data.position.z + data.size.y * 0.5f};
        DrawLine3D(p1, p2, YELLOW);
        DrawLine3D(p2, p3, YELLOW);
        DrawLine3D(p3, p4, YELLOW);
        DrawLine3D(p4, p1, YELLOW);
        break;
    }
    default:
        break;
    }
}






