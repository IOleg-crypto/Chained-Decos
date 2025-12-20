#ifndef EDITOR_RENDERER_H
#define EDITOR_RENDERER_H

#include "scene/resources/map/core/MapData.h"
#include <raylib.h>

#include "editor/IEditor.h"
#include "editor/tool/IToolManager.h"

// EditorRenderer - handles all rendering operations for the editor
class EditorRenderer
{
public:
    EditorRenderer(IEditor *editor, IToolManager *toolManager);
    ~EditorRenderer() = default;

    // Render a map object with selection indicators and gizmos
    void RenderObject(const MapObjectData &data, bool isSelected);

    // Render transformation gizmo for selected objects
    void RenderGizmo(const MapObjectData &data);

    // Render spawn zone with texture
    void RenderSpawnZoneWithTexture(Texture2D texture, const Vector3 &position, Vector3 scale,
                                    Color color, bool textureLoaded);

private:
    // Render selection wireframe for different object types
    void RenderSelectionWireframe(const MapObjectData &data);

    // Helper references (not owned)
    IEditor *m_editor;
    IToolManager *m_toolManager;
};

#endif // EDITOR_RENDERER_H
