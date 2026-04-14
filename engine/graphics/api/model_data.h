#ifndef CH_MODEL_DATA_H
#define CH_MODEL_DATA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <string>
#include <vector>

namespace CHEngine
{
struct TransformData
{
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

struct RawMesh
{
    std::vector<float> vertices;
    std::vector<float> texcoords;
    std::vector<float> normals;
    std::vector<float> tangents;
    std::vector<unsigned char> colors;
    std::vector<uint32_t> indices;

    // Skinning data
    std::vector<unsigned char> joints; // 4 joints per vertex
    std::vector<float> weights;        // 4 weights per vertex

    int materialIndex = -1;
};

struct RawMaterial
{
    std::string albedoPath;
    glm::vec4 albedoColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    std::string emissivePath;
    glm::vec4 emissiveColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    float emissiveIntensity = 0.0f;

    std::string normalPath;
    std::string metallicRoughnessPath;
    std::string occlusionPath;

    float metalness = 0.0f;
    float roughness = 0.5f;
};

struct EmbeddedTextureData
{
    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool isHDR = false;
};

// Track textures that are still loading
struct PendingTexture
{
    int materialIndex;
    std::string path;
    int mapIndex; // MATERIAL_MAP_ALBEDO, MATERIAL_MAP_EMISSION, etc.
};

struct RawAnimation
{
    std::string name;
    int frameCount;
    int boneCount;
    float frameRate = 30.0f; // FPS of the animation source asset
    std::vector<TransformData> framePoses; // flattened [frameCount * boneCount]
};

struct MeshInstance
{
    int meshIndex;
    glm::mat4 localTransform;
};

struct BoneInfoData
{
    char name[32];
    int parent;
};

// CPU-side data for async loading (loaded in worker thread)
struct PendingModelData
{
    std::string fullPath;
    std::vector<RawMesh> meshes;
    std::vector<RawMaterial> materials;
    std::unordered_map<std::string, EmbeddedTextureData> embeddedTextures;

    // Skeletal / Hierarchy data (Original data for animations only)
    std::vector<BoneInfoData> bones;
    std::vector<TransformData> bindPose;

    // Flattened render data
    std::vector<MeshInstance> instances;
    
    // Support for hierarchy (only for animation system/skeleton mapping)
    std::vector<std::string> nodeNames;
    int nodeCount = 0; // Added for convenience
    std::vector<int> nodeParents;
    std::vector<glm::mat4> nodeLocalTransforms;
    std::vector<glm::mat4> globalBindPoses; 
    std::vector<glm::mat4> offsetMatrices;  
    
    std::vector<RawAnimation> animations;
    bool isValid = false;
};
} // namespace CHEngine

#endif // CH_MODEL_DATA_H
