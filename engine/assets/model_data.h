#ifndef CH_MODEL_DATA_H
#define CH_MODEL_DATA_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <string>
#include <vector>

#include <cereal/cereal.hpp>



// GLM serialization helpers for Cereal
namespace glm {
    template<class Archive>
    void serialize(Archive& archive, vec3& v) { archive(v.x, v.y, v.z); }
    template<class Archive>
    void serialize(Archive& archive, vec4& v) { archive(v.x, v.y, v.z, v.w); }
    template<class Archive>
    void serialize(Archive& archive, quat& q) { archive(q.x, q.y, q.z, q.w); }
    template<class Archive>
    void serialize(Archive& archive, mat4& m) {
        archive(m[0], m[1], m[2], m[3]);
    }
}

namespace Chained
{
struct BoundingBox
{
    glm::vec3 Min;
    glm::vec3 Max;
};
struct TransformData
{
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(translation, rotation, scale);
    }
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

    glm::vec3 MinBounds = { 0, 0, 0 };
    glm::vec3 MaxBounds = { 0, 0, 0 };

    template<class Archive>
    void serialize(Archive& archive) {
        archive(vertices, texcoords, normals, tangents, colors, indices, joints, weights, materialIndex, MinBounds, MaxBounds);
    }
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

    bool transparent = false;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(albedoPath, albedoColor, emissivePath, emissiveColor, emissiveIntensity, normalPath, metallicRoughnessPath, occlusionPath, metalness, roughness, transparent);
    }
};

struct EmbeddedTextureData
{
    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    bool isHDR = false;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(data, width, height, channels, isHDR);
    }
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

    template<class Archive>
    void serialize(Archive& archive) {
        archive(name, frameCount, boneCount, frameRate, framePoses);
    }
};

struct MeshInstance
{
    int meshIndex;
    glm::mat4 localTransform;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(meshIndex, localTransform);
    }
};

struct BoneInfoData
{
    std::string name;
    int parent;

    template<class Archive>
    void serialize(Archive& archive) {
        archive(name, parent);
    }
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
    int FinalizationProgress = 0; // Index of the next mesh to be finalized
    bool isValid = false;
    
    template<class Archive>
    void serialize(Archive& archive) {
        archive(fullPath, meshes, materials, embeddedTextures, bones, bindPose, instances, nodeNames, nodeCount, nodeParents, nodeLocalTransforms, globalBindPoses, offsetMatrices, animations, isValid);
    }
};
} // namespace Chained

#endif // CH_MODEL_DATA_H
