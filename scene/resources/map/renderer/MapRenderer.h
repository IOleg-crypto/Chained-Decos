#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include "../core/MapData.h"
#include "../core/MapLoader.h"
#include <raylib.h>
#include <unordered_map>
#include <string>

// Forward declarations
class Model;

// MapRenderer - handles all rendering operations for maps
class MapRenderer
{
public:
    MapRenderer() = default;
    ~MapRenderer() = default;

    // Render entire map with skybox and all objects
    void RenderMap(const GameMap& map, Camera3D camera);

    // Render single map object
    void RenderMapObject(const MapObjectData& object,
                        const std::unordered_map<std::string, Model>& loadedModels,
                        Camera3D camera,
                        bool useEditorColors = false);

    // Render spawn zone with texture
    void RenderSpawnZone(Texture2D spawnTexture, const Vector3& position, float size, Color color, bool textureLoaded) const;

private:
    // Helper to render spawn zone texture using RenderUtils
    void RenderSpawnZoneWithTexture(Texture2D texture, const Vector3& position, float size, Color color, bool textureLoaded) const;
};

#endif // MAP_RENDERER_H







