#include "engine/assets/loaders/model_loader.h"
#include "engine/assets/loaders/assimp_importer.h"

namespace Chained
{
std::shared_ptr<Asset> ModelLoader::Create()
{
    return std::make_shared<ModelAsset>();
}

bool ModelLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError)
{
    auto modelAsset = std::static_pointer_cast<ModelAsset>(asset);

    // Handle procedural models
    if (resolvedPath.starts_with(":"))
    {
        // Placeholder: GenerateProceduralModel was removed from ModelLoader in favor of AssimpImporter
        // If you need it, move it to AssimpImporter or keep a simplified version here
        if (outError)
        {
            *outError = "ModelLoader: procedural model paths are not supported: " + resolvedPath;
        }
        return false;
    }

    auto pendingData = LoadMeshDataFromDisk(resolvedPath);
    if (pendingData.isValid)
    {
        modelAsset->SetPendingData(std::move(pendingData));
        return true;
    }
    if (outError)
    {
        *outError = "ModelLoader: failed to import model data from '" + resolvedPath + "'";
    }
    return false;
}

PendingModelData ModelLoader::LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS)
{
    return AssimpImporter::Import(path, samplingFPS);
}

Model ModelLoader::GenerateProceduralModel(const std::string& type, const ProceduralParameters& params)
{
    Model model;
    RawMesh raw;

    if (type == ":cube:")
    {
        float w = params.Dimensions.x * 0.5f;
        float h = params.Dimensions.y * 0.5f;
        float d = params.Dimensions.z * 0.5f;

        raw.vertices = {
            -w,-h, d,  w,-h, d,  w, h, d, -w, h, d,
            -w,-h,-d, -w, h,-d,  w, h,-d,  w,-h,-d,
            -w, h,-d, -w, h, d,  w, h, d,  w, h,-d,
            -w,-h,-d,  w,-h,-d,  w,-h, d, -w,-h, d,
             w,-h,-d,  w, h,-d,  w, h, d,  w,-h, d,
            -w,-h,-d, -w,-h, d, -w, h, d, -w, h,-d
        };

        raw.normals = {
             0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1,
             0, 0,-1,   0, 0,-1,   0, 0,-1,   0, 0,-1,
             0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0,
             0,-1, 0,   0,-1, 0,   0,-1, 0,   0,-1, 0,
             1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0,
            -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0
        };
        
        std::vector<uint32_t> indices;
        for(int i=0; i<6; i++) {
            indices.push_back(i*4+0); indices.push_back(i*4+1); indices.push_back(i*4+2);
            indices.push_back(i*4+0); indices.push_back(i*4+2); indices.push_back(i*4+3);
        }

        Mesh mesh;
        mesh.MaterialIndex = 0;
        mesh.VertexCount = (uint32_t)(raw.vertices.size() / 3);
        mesh.TriangleCount = (uint32_t)(indices.size() / 3);
        
        mesh.VAO = VertexArray::Create();
        auto vb = VertexBuffer::Create(raw.vertices.data(), (uint32_t)(raw.vertices.size() * sizeof(float)));
        vb->SetLayout({
            { ShaderDataType::Float3, "a_Position" }
        });
        mesh.VAO->AddVertexBuffer(vb);

        auto ib = IndexBuffer::Create(indices.data(), (uint32_t)indices.size());
        mesh.VAO->SetIndexBuffer(ib);

        model.Meshes.push_back(mesh);
        model.Materials.push_back(Material{});
    }

    return model;
}
} // namespace CHEngine