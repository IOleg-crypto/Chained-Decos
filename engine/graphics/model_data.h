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
        std::vector<unsigned char> colors;
        std::vector<unsigned short> indices;
        int materialIndex = -1;
    };

    struct RawMaterial {
        std::string albedoPath;
        Color albedoColor = WHITE;
    };

    // Track textures that are still loading
    struct PendingTexture {
        int materialIndex;
        std::string path;
    };

    // CPU-side data for async loading (loaded in worker thread)
    struct PendingModelData {
        std::string fullPath;   
        std::vector<RawMesh> meshes;
        std::vector<RawMaterial> materials;
        
        ModelAnimation* animations = nullptr;
        int animationCount = 0;
        bool isValid = false;
    };
}

#endif // CH_MODEL_DATA_H
