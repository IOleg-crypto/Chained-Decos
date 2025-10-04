#include "TerrainEditor.h"
#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <functional>

TerrainEditor::TerrainEditor()
    : m_terrainGenerated(false)
    , m_currentBrush(BrushType::RAISE_LOWER)
    , m_brushSize(5.0f)
    , m_brushStrength(1.0f)
    , m_brushFalloff(0.5f)
    , m_brushActive(false)
    , m_selectedTextureLayer(0)
    , m_targetHeight(0.0f)
    , m_terrainPosition({0.0f, 0.0f, 0.0f})
    , m_terrainScale({1.0f, 1.0f, 1.0f})
    , m_terrainResolution(256)
    , m_heightScale(10.0f)
{
}

TerrainEditor::~TerrainEditor()
{
    Cleanup();
}

bool TerrainEditor::Initialize(int resolution)
{
    m_terrainResolution = resolution;

    // Initialize heightmap data
    m_heightmap.width = resolution;
    m_heightmap.height = resolution;
    m_heightmap.heights.resize(resolution * resolution, 0.0f);
    m_heightmap.minHeight = 0.0f;
    m_heightmap.maxHeight = 0.0f;

    // Generate flat terrain by default
    for (int z = 0; z < resolution; ++z)
    {
        for (int x = 0; x < resolution; ++x)
        {
            float height = 0.0f; // Start flat
            m_heightmap.SetHeight(x, z, height);
        }
    }

    return GenerateTerrain();
}

void TerrainEditor::Cleanup()
{
    if (m_terrainGenerated)
    {
        UnloadModel(m_terrainModel);
        m_terrainGenerated = false;
    }

    for (auto& layer : m_textureLayers)
    {
        if (layer.loaded)
        {
            UnloadTexture(layer.texture);
            layer.loaded = false;
        }
    }
    m_textureLayers.clear();
}

bool TerrainEditor::GenerateTerrain()
{
    if (m_terrainGenerated)
    {
        UnloadModel(m_terrainModel);
    }

    // Create image from heightmap data for GenMeshHeightmap
    Image heightmapImage = {
        .data = m_heightmap.heights.data(),
        .width = m_heightmap.width,
        .height = m_heightmap.height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R32
    };

    // Create terrain mesh from heightmap
    m_terrainMesh = GenMeshHeightmap(heightmapImage, {m_terrainScale.x, m_heightScale, m_terrainScale.z});

    // Update normals for proper lighting
    UpdateTerrainNormals();

    // Create model from mesh
    m_terrainModel = LoadModelFromMesh(m_terrainMesh);

    m_terrainGenerated = true;
    return true;
}

bool TerrainEditor::LoadHeightmap(const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open heightmap file: " << filePath << std::endl;
        return false;
    }

    // Read raw heightmap data (assuming 16-bit grayscale)
    std::vector<uint16_t> rawData(m_terrainResolution * m_terrainResolution);
    file.read(reinterpret_cast<char*>(rawData.data()), rawData.size() * sizeof(uint16_t));

    if (file.gcount() != rawData.size() * sizeof(uint16_t))
    {
        std::cerr << "Invalid heightmap file size" << std::endl;
        return false;
    }

    // Convert to float heights
    for (int z = 0; z < m_terrainResolution; ++z)
    {
        for (int x = 0; x < m_terrainResolution; ++x)
        {
            float height = rawData[z * m_terrainResolution + x] / 65535.0f * m_heightScale;
            m_heightmap.SetHeight(x, z, height);
        }
    }

    m_currentHeightmapFile = filePath;
    return GenerateTerrain();
}

bool TerrainEditor::SaveHeightmap(const std::string& filePath)
{
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to create heightmap file: " << filePath << std::endl;
        return false;
    }

    // Convert float heights to 16-bit format
    std::vector<uint16_t> rawData(m_terrainResolution * m_terrainResolution);
    for (int z = 0; z < m_terrainResolution; ++z)
    {
        for (int x = 0; x < m_terrainResolution; ++x)
        {
            float normalizedHeight = m_heightmap.GetHeight(x, z) / m_heightScale;
            normalizedHeight = std::max(0.0f, std::min(1.0f, normalizedHeight));
            rawData[z * m_terrainResolution + x] = static_cast<uint16_t>(normalizedHeight * 65535.0f);
        }
    }

    file.write(reinterpret_cast<const char*>(rawData.data()), rawData.size() * sizeof(uint16_t));
    m_currentHeightmapFile = filePath;
    return true;
}

bool TerrainEditor::LoadTextureLayer(const std::string& texturePath, int layerIndex)
{
    // Unload existing texture if it exists
    if (layerIndex >= 0 && layerIndex < static_cast<int>(m_textureLayers.size()))
    {
        if (m_textureLayers[layerIndex].loaded)
        {
            UnloadTexture(m_textureLayers[layerIndex].texture);
        }
    }
    else
    {
        // Add new layer
        m_textureLayers.push_back(TerrainTextureLayer());
        layerIndex = static_cast<int>(m_textureLayers.size()) - 1;
    }

    TerrainTextureLayer& layer = m_textureLayers[layerIndex];
    layer.texture = LoadTexture(texturePath.c_str());
    layer.loaded = layer.texture.id != 0;
    layer.texturePath = texturePath;

    if (layer.loaded)
    {
        std::cout << "Loaded texture layer: " << texturePath << std::endl;
        return true;
    }
    else
    {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
        return false;
    }
}

void TerrainEditor::ApplyBrush(int worldX, int worldZ)
{
    if (!m_brushActive) return;

    Vector2 hmCoords = WorldToHeightmapCoords(worldX, worldZ);
    int centerX = static_cast<int>(hmCoords.x);
    int centerZ = static_cast<int>(hmCoords.y);

    switch (m_currentBrush)
    {
        case BrushType::RAISE_LOWER:
            ApplyHeightBrush(centerX, centerZ);
            break;
        case BrushType::SMOOTH:
            ApplySmoothBrush(centerX, centerZ);
            break;
        case BrushType::FLATTEN:
            ApplyFlattenBrush(centerX, centerZ);
            break;
        case BrushType::NOISE:
            ApplyNoiseBrush(centerX, centerZ);
            break;
        case BrushType::SET_HEIGHT:
            ApplySetHeightBrush(centerX, centerZ);
            break;
        case BrushType::PAINT_TEXTURE:
            PaintTexture(worldX, worldZ);
            break;
    }

    // Regenerate terrain mesh after modifications
    GenerateTerrain();
}

void TerrainEditor::SetBrushType(BrushType type)
{
    m_currentBrush = type;
}

void TerrainEditor::SetBrushSize(float size)
{
    m_brushSize = std::max(0.1f, size);
}

void TerrainEditor::SetBrushStrength(float strength)
{
    m_brushStrength = std::max(0.0f, std::min(1.0f, strength));
}

void TerrainEditor::SetBrushFalloff(float falloff)
{
    m_brushFalloff = std::max(0.0f, std::min(1.0f, falloff));
}

void TerrainEditor::PaintTexture(int worldX, int worldZ)
{
    // Texture painting implementation would go here
    // For now, just a placeholder
}

void TerrainEditor::SetSelectedTextureLayer(int layer)
{
    if (layer >= 0 && layer < static_cast<int>(m_textureLayers.size()))
    {
        m_selectedTextureLayer = layer;
    }
}

void TerrainEditor::SetTerrainPosition(const Vector3& position)
{
    m_terrainPosition = position;
}

void TerrainEditor::SetTerrainScale(const Vector3& scale)
{
    m_terrainScale = scale;
}

void TerrainEditor::SetHeightScale(float scale)
{
    m_heightScale = scale;
}

void TerrainEditor::SetTargetHeight(float height)
{
    m_targetHeight = height;
}

Vector3 TerrainEditor::GetTerrainHeight(const Vector3& position) const
{
    Vector3 localPos = {
        position.x - m_terrainPosition.x,
        0.0f,
        position.z - m_terrainPosition.z
    };

    Vector2 hmCoords = {
        (localPos.x / m_terrainScale.x) * (m_terrainResolution - 1),
        (localPos.z / m_terrainScale.z) * (m_terrainResolution - 1)
    };

    int x0 = static_cast<int>(std::floor(hmCoords.x));
    int z0 = static_cast<int>(std::floor(hmCoords.y));
    int x1 = std::min(x0 + 1, m_terrainResolution - 1);
    int z1 = std::min(z0 + 1, m_terrainResolution - 1);

    float fx = hmCoords.x - x0;
    float fz = hmCoords.y - z0;

    float h00 = m_heightmap.GetHeight(x0, z0);
    float h01 = m_heightmap.GetHeight(x0, z1);
    float h10 = m_heightmap.GetHeight(x1, z0);
    float h11 = m_heightmap.GetHeight(x1, z1);

    float height = h00 * (1 - fx) * (1 - fz) +
                   h10 * fx * (1 - fz) +
                   h01 * (1 - fx) * fz +
                   h11 * fx * fz;

    return {position.x, m_terrainPosition.y + height * m_terrainScale.y, position.z};
}

bool TerrainEditor::IsPointOnTerrain(const Vector3& point) const
{
    Vector3 localPos = {
        point.x - m_terrainPosition.x,
        0.0f,
        point.z - m_terrainPosition.z
    };

    return localPos.x >= 0 && localPos.x <= m_terrainScale.x &&
           localPos.z >= 0 && localPos.z <= m_terrainScale.z;
}

void TerrainEditor::FlattenArea(const Rectangle& area, float height)
{
    int startX = static_cast<int>((area.x / m_terrainScale.x) * m_terrainResolution);
    int startZ = static_cast<int>((area.y / m_terrainScale.z) * m_terrainResolution);
    int endX = static_cast<int>(((area.x + area.width) / m_terrainScale.x) * m_terrainResolution);
    int endZ = static_cast<int>(((area.y + area.height) / m_terrainScale.z) * m_terrainResolution);

    startX = std::max(0, std::min(startX, m_terrainResolution - 1));
    startZ = std::max(0, std::min(startZ, m_terrainResolution - 1));
    endX = std::max(0, std::min(endX, m_terrainResolution - 1));
    endZ = std::max(0, std::min(endZ, m_terrainResolution - 1));

    for (int z = startZ; z <= endZ; ++z)
    {
        for (int x = startX; x <= endX; ++x)
        {
            m_heightmap.SetHeight(x, z, height);
        }
    }
}

void TerrainEditor::SmoothArea(const Rectangle& area)
{
    // Smooth area implementation would go here
    // For now, just a placeholder
}

void TerrainEditor::AddNoise(const Rectangle& area, float noiseStrength)
{
    // Add noise implementation would go here
    // For now, just a placeholder
}

void TerrainEditor::Render()
{
    if (m_terrainGenerated)
    {
        DrawModel(m_terrainModel, m_terrainPosition, 1.0f, WHITE);
    }
}

void TerrainEditor::RenderBrushPreview()
{
    if (!m_brushActive) return;

    // Draw brush circle preview
    DrawCircle3D({0, 0, 0}, m_brushSize, {1, 0, 0}, 90.0f, YELLOW);
}

void TerrainEditor::UpdateTerrainMesh()
{
    // Update terrain mesh from heightmap data
    UpdateMeshBuffer(m_terrainMesh, 0, m_heightmap.heights.data(),
                    m_heightmap.heights.size() * sizeof(float), 0);
}

void TerrainEditor::UpdateTerrainNormals()
{
    // Calculate normals for the terrain mesh
    std::vector<Vector3> normals(m_terrainResolution * m_terrainResolution);

    for (int z = 0; z < m_terrainResolution; ++z)
    {
        for (int x = 0; x < m_terrainResolution; ++x)
        {
            // Calculate normal using neighboring heights
            float heightL = (x > 0) ? m_heightmap.GetHeight(x - 1, z) : m_heightmap.GetHeight(x, z);
            float heightR = (x < m_terrainResolution - 1) ? m_heightmap.GetHeight(x + 1, z) : m_heightmap.GetHeight(x, z);
            float heightD = (z > 0) ? m_heightmap.GetHeight(x, z - 1) : m_heightmap.GetHeight(x, z);
            float heightU = (z < m_terrainResolution - 1) ? m_heightmap.GetHeight(x, z + 1) : m_heightmap.GetHeight(x, z);

            Vector3 normal = {
                -(heightR - heightL) * m_heightScale,
                2.0f,
                -(heightU - heightD) * m_heightScale
            };

            normals[z * m_terrainResolution + x] = Vector3Normalize(normal);
        }
    }

    UpdateMeshBuffer(m_terrainMesh, 2, normals.data(), normals.size() * sizeof(Vector3), 0);
}

float TerrainEditor::GetBrushWeight(float distance) const
{
    if (distance >= m_brushSize) return 0.0f;

    float normalizedDistance = distance / m_brushSize;
    if (normalizedDistance <= m_brushFalloff)
    {
        return 1.0f;
    }
    else
    {
        float falloffFactor = 1.0f - ((normalizedDistance - m_brushFalloff) / (1.0f - m_brushFalloff));
        return std::max(0.0f, falloffFactor);
    }
}

Vector2 TerrainEditor::WorldToHeightmapCoords(int worldX, int worldZ) const
{
    float x = (worldX - m_terrainPosition.x) / m_terrainScale.x * (m_terrainResolution - 1);
    float z = (worldZ - m_terrainPosition.z) / m_terrainScale.z * (m_terrainResolution - 1);
    return {x, z};
}

void TerrainEditor::ApplyBrushToArea(int centerX, int centerZ, std::function<void(int, int, float)> applyEffect)
{
    int radius = static_cast<int>(m_brushSize);
    int startX = std::max(0, centerX - radius);
    int endX = std::min(m_terrainResolution - 1, centerX + radius);
    int startZ = std::max(0, centerZ - radius);
    int endZ = std::min(m_terrainResolution - 1, centerZ + radius);

    for (int z = startZ; z <= endZ; ++z) {
        for (int x = startX; x <= endX; ++x) {
            float dx = static_cast<float>(x - centerX);
            float dz = static_cast<float>(z - centerZ);
            float distance = std::sqrt(dx * dx + dz * dz);

            float weight = GetBrushWeight(distance);
            if (weight > 0.0f) {
                applyEffect(x, z, weight);
            }
        }
    }
}

void TerrainEditor::ApplyHeightBrush(int centerX, int centerZ)
{
    int brushRadius = static_cast<int>(m_brushSize);
    int startX = std::max(0, centerX - brushRadius);
    int endX = std::min(m_terrainResolution - 1, centerX + brushRadius);
    int startZ = std::max(0, centerZ - brushRadius);
    int endZ = std::min(m_terrainResolution - 1, centerZ + brushRadius);

    for (int z = startZ; z <= endZ; ++z)
    {
        for (int x = startX; x <= endX; ++x)
        {
            float distance = std::sqrt(static_cast<float>((x - centerX) * (x - centerX) +
                                                        (z - centerZ) * (z - centerZ)));
            float weight = GetBrushWeight(distance);
            if (weight > 0.0f)
            {
                float currentHeight = m_heightmap.GetHeight(x, z);
                float newHeight = currentHeight + m_brushStrength * weight;
                m_heightmap.SetHeight(x, z, newHeight);
            }
        }
    }
}

void TerrainEditor::ApplySmoothBrush(int centerX, int centerZ)
{
    ApplyBrushToArea(centerX, centerZ, [&](int x, int z, float weight) {
        float sum = 0.0f;
        int count = 0;

        for (int dz = -1; dz <= 1; ++dz) {
            for (int dx = -1; dx <= 1; ++dx) {
                int nx = x + dx;
                int nz = z + dz;
                if (nx >= 0 && nx < m_terrainResolution && nz >= 0 && nz < m_terrainResolution) {
                    sum += m_heightmap.GetHeight(nx, nz);
                    count++;
                }
            }
        }

        if (count > 0) {
            float averageHeight = sum / count;
            float currentHeight = m_heightmap.GetHeight(x, z);
            float smoothedHeight = currentHeight * (1.0f - weight) + averageHeight * weight;
            m_heightmap.SetHeight(x, z, smoothedHeight);
        }
    });
}

void TerrainEditor::ApplyFlattenBrush(int centerX, int centerZ)
{
    ApplyBrushToArea(centerX, centerZ, [&](int x, int z, float weight) {
        float currentHeight = m_heightmap.GetHeight(x, z);
        float flattenedHeight = currentHeight * (1.0f - weight) + m_targetHeight * weight;
        m_heightmap.SetHeight(x, z, flattenedHeight);
    });
}

void TerrainEditor::ApplyNoiseBrush(int centerX, int centerZ)
{
    // Noise brush implementation would add random variation
    // For now, just a placeholder
}

void TerrainEditor::ApplySetHeightBrush(int centerX, int centerZ)
{
    ApplyBrushToArea(centerX, centerZ, [&](int x, int z, float weight) {
        m_heightmap.SetHeight(x, z, m_targetHeight);
    });
}