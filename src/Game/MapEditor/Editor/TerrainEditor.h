#ifndef TERRAINEDITOR_H
#define TERRAINEDITOR_H

#include <raylib.h>
#include <vector>
#include <string>
#include <memory>
#include <functional>

#include "MapObject.h"

// Terrain brush types
enum class BrushType
{
    RAISE_LOWER = 0,
    SMOOTH = 1,
    FLATTEN = 2,
    NOISE = 3,
    SET_HEIGHT = 4,
    PAINT_TEXTURE = 5
};

// Terrain texture layers
struct TerrainTextureLayer
{
    std::string texturePath;
    Texture2D texture;
    bool loaded;
    float opacity;
    Vector2 scale;

    TerrainTextureLayer() : loaded(false), opacity(1.0f), scale({1.0f, 1.0f}) {}
};

// Heightmap data structure
struct HeightmapData
{
    std::vector<float> heights;
    int width;
    int height;
    float minHeight;
    float maxHeight;

    HeightmapData() : width(0), height(0), minHeight(0.0f), maxHeight(0.0f) {}

    float GetHeight(int x, int z) const
    {
        if (x < 0 || x >= width || z < 0 || z >= height)
            return 0.0f;
        return heights[z * width + x];
    }

    void SetHeight(int x, int z, float height)
    {
        if (x < 0 || x >= width || z < 0 || z >= height)
            return;
        heights[z * width + x] = height;
        if (height < minHeight) minHeight = height;
        if (height > maxHeight) maxHeight = height;
    }
};

// Terrain editor for heightmap-based terrain editing
class TerrainEditor
{
private:
    // Terrain data
    HeightmapData m_heightmap;
    std::vector<TerrainTextureLayer> m_textureLayers;
    Mesh m_terrainMesh;
    Model m_terrainModel;
    bool m_terrainGenerated;

    // Brush settings
    BrushType m_currentBrush;
    float m_brushSize;
    float m_brushStrength;
    float m_brushFalloff;
    bool m_brushActive;

    // Texture painting
    int m_selectedTextureLayer;
    float m_targetHeight;

    // Terrain settings
    Vector3 m_terrainPosition;
    Vector3 m_terrainScale;
    int m_terrainResolution;
    float m_heightScale;

    // File operations
    std::string m_currentHeightmapFile;
    std::string m_currentTextureDir;

public:
    TerrainEditor();
    ~TerrainEditor();

    // Initialization
    bool Initialize(int resolution = 256);
    void Cleanup();

    // Terrain generation
    bool GenerateTerrain();
    bool LoadHeightmap(const std::string& filePath);
    bool SaveHeightmap(const std::string& filePath);
    bool LoadTextureLayer(const std::string& texturePath, int layerIndex = -1);

    // Brush operations
    void ApplyBrush(int worldX, int worldZ);
    void SetBrushType(BrushType type);
    void SetBrushSize(float size);
    void SetBrushStrength(float strength);
    void SetBrushFalloff(float falloff);

    // Texture operations
    void PaintTexture(int worldX, int worldZ);
    void SetSelectedTextureLayer(int layer);

    // Terrain properties
    void SetTerrainPosition(const Vector3& position);
    void SetTerrainScale(const Vector3& scale);
    void SetHeightScale(float scale);
    void SetTargetHeight(float height);

    // Getters
    HeightmapData& GetHeightmap() { return m_heightmap; }
    const HeightmapData& GetHeightmap() const { return m_heightmap; }
    Model& GetTerrainModel() { return m_terrainModel; }
    BrushType GetCurrentBrush() const { return m_currentBrush; }
    float GetBrushSize() const { return m_brushSize; }
    float GetBrushStrength() const { return m_brushStrength; }
    bool IsBrushActive() const { return m_brushActive; }

    // Utility functions
    Vector3 GetTerrainHeight(const Vector3& position) const;
    bool IsPointOnTerrain(const Vector3& point) const;
    void FlattenArea(const Rectangle& area, float height);
    void SmoothArea(const Rectangle& area);
    void AddNoise(const Rectangle& area, float noiseStrength);

    // Rendering
    void Render();
    void RenderBrushPreview();

private:
    // Internal helper functions
    void UpdateTerrainMesh();
    void UpdateTerrainNormals();
    float GetBrushWeight(float distance) const;
    Vector2 WorldToHeightmapCoords(int worldX, int worldZ) const;
    void ApplyBrushToArea(int centerX, int centerZ, std::function<void(int, int, float)> applyEffect);
    void ApplyHeightBrush(int centerX, int centerZ);
    void ApplySmoothBrush(int centerX, int centerZ);
    void ApplyFlattenBrush(int centerX, int centerZ);
    void ApplyNoiseBrush(int centerX, int centerZ);
    void ApplySetHeightBrush(int centerX, int centerZ);
};

#endif // TERRAINEDITOR_H