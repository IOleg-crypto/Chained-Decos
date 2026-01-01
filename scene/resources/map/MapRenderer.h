#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include "MapData.h"
#include "SceneLoader.h"
#include <raylib.h>
#include <string>
#include <unordered_map>

namespace CHEngine
{

class GameScene;

// MapRenderer - handles all rendering operations for maps
class MapRenderer
{
public:
    MapRenderer() = default;
    ~MapRenderer() = default;

    // Render entire map with skybox and all objects (Manages its own 3D mode)
    void RenderMap(const GameScene &map, Camera3D camera);

    // Render only map content (Should be called between BeginMode3D/EndMode3D)
    void DrawMapContent(const GameScene &map, Camera3D camera, bool hideSpawnZones = false);

    // Render single map object
    void RenderMapObject(const MapObjectData &object,
                         const std::unordered_map<std::string, Model> &loadedModels,
                         const std::unordered_map<std::string, Texture2D> &loadedTextures,
                         Camera3D camera, bool useEditorColors = false, bool wireframe = false);

    // Render spawn zone with texture
    void RenderSpawnZone(Texture2D spawnTexture, const Vector3 &position, float size, Color color,
                         bool textureLoaded) const;

private:
    // Helper to render spawn zone texture using RenderUtils
    void RenderSpawnZoneWithTexture(Texture2D texture, const Vector3 &position, float size,
                                    Color color, bool textureLoaded) const;
};

} // namespace CHEngine

#endif // MAP_RENDERER_H
