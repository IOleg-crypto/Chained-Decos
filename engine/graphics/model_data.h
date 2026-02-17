#ifndef CH_MODEL_DATA_H
#define CH_MODEL_DATA_H

#include <string>
#include <vector>
#include "raylib.h"

namespace CHEngine
{
    struct RawMesh {
        std::vector<float> vertices;
        std::vector<float> texcoords;
        std::vector<float> normals;
        std::vector<float> tangents;
        std::vector<unsigned char> colors;
        std::vector<unsigned short> indices;
        
        // Skinning data
        std::vector<unsigned char> joints;  // 4 joints per vertex
        std::vector<float> weights;        // 4 weights per vertex

        int materialIndex = -1;
    };

    struct RawMaterial {
        std::string albedoPath;
        Color albedoColor = WHITE;

        std::string emissivePath;
        Color emissiveColor = BLACK;
        float emissiveIntensity = 0.0f;

        std::string normalPath;
        std::string metallicRoughnessPath;
        std::string occlusionPath;

        float metalness = 0.0f;
        float roughness = 0.5f;
    };

    // Track textures that are still loading
    struct PendingTexture {
        int materialIndex;
        std::string path;
        int mapIndex; // MATERIAL_MAP_ALBEDO, MATERIAL_MAP_EMISSION, etc.
    };

    struct RawAnimation {
        std::string name;
        int frameCount;
        int boneCount;
        std::vector<Transform> framePoses; // flattened [frameCount * boneCount]
    };

    // CPU-side data for async loading (loaded in worker thread)
    struct PendingModelData {
        std::string fullPath;   
        std::vector<RawMesh> meshes;
        std::vector<RawMaterial> materials;
        
        // Skeletal / Hierarchy data (RAII)
        std::vector<BoneInfo> bones;
        std::vector<Transform> bindPose; 
        
        // KISS additions
        std::vector<std::string> nodeNames;
        std::vector<int> nodeParents;
        std::vector<Matrix> nodeLocalTransforms;
        std::vector<Matrix> globalBindPoses; // Computed world-space bind poses
        std::vector<Matrix> offsetMatrices;  // per-bone offset matrices (Inverse Bind)
        std::vector<int> meshToNode;

        std::vector<RawAnimation> animations;
        bool isValid = false;
    };
}

#endif // CH_MODEL_DATA_H
