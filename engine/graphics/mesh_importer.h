#ifndef CH_MESH_IMPORTER_H
#define CH_MESH_IMPORTER_H

#include "asset_importer.h"
#include "model_data.h"

struct cgltf_data;

namespace CHEngine
{
    class ModelAsset;

    // Helpers from original ModelAsset.cpp
    struct GLTFParserContext {
        PendingModelData& data;
        cgltf_data* gltf;
    };

    class MeshImporter : public AssetImporter
    {
    public:
        static std::shared_ptr<ModelAsset> ImportMesh(const std::filesystem::path& path);
        
        // For procedural mesh generation
        static Model GenerateProceduralModel(const std::string& type);

        // For async loading
        static PendingModelData LoadMeshDataFromDisk(const std::filesystem::path& path);
    };
}

#endif // CH_MESH_IMPORTER_H
