#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

#include "../Object/MapObject.h"
#include "Engine/Map/Core/MapLoader.h"
#include "Engine/Map/Core/MapData.h"
#include "../ToolManager/IToolManager.h"
#include "../CameraManager/ICameraManager.h"
#include "../ModelManager/IModelManager.h"
#include <raylib.h>
#include <unordered_map>
#include <string>
#include "Engine/Model/Core/Model.h"

// EditorRenderer - handles all rendering operations for the editor
class EditorRenderer
{
public:
    EditorRenderer(IToolManager* toolManager, ICameraManager* cameraManager, IModelManager* modelManager);
    ~EditorRenderer() = default;

    // Render a map object with selection indicators and gizmos
    void RenderObject(const MapObject& obj, const MapObjectData& data, bool isSelected);

    // Render transformation gizmo for selected objects
    void RenderGizmo(const MapObject& obj, const MapObjectData& data);

    // Render spawn zone with texture
    void RenderSpawnZoneWithTexture(Texture2D texture, const Vector3& position, float size, Color color, bool textureLoaded);

private:
    // Render selection wireframe for different object types
    void RenderSelectionWireframe(const MapObjectData& data);

    // Helper references (not owned)
    IToolManager* m_toolManager;
    ICameraManager* m_cameraManager;
    IModelManager* m_modelManager;
};

#endif // EDITOR_RENDERER_H

