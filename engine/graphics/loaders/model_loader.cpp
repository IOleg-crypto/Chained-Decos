#include "engine/graphics/loaders/model_loader.h"
#include "engine/graphics/loaders/assimp_importer.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/scene/project.h"
#include "engine/graphics/loaders/model_cache.h"
#include <glm/gtc/type_ptr.hpp>

namespace CHEngine
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
    if (ModelCache::IsCacheValid(path))
    {
        PendingModelData data;
        if (ModelCache::Load(ModelCache::GetCachePath(path), data))
        {
            CH_CORE_INFO("ModelLoader: Loaded cached model data for '{}'", path.generic_string());
            return data;
        }
    }

    auto data = AssimpImporter::Import(path, samplingFPS);
    if (data.isValid)
    {
        CH_CORE_INFO("ModelLoader: Saving model cache for '{}'", path.generic_string());
        ModelCache::Save(ModelCache::GetCachePath(path), data);
    }
    return data;
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

void ModelLoader::Finalize(std::shared_ptr<ModelAsset> asset)
{
    CH_PROFILE_FUNCTION();

    if (!asset->m_HasPendingData || !asset->m_PendingData.isValid)
    {
        return;
    }

    CH_CORE_INFO("ModelAsset: Uploading model to GPU: '{}' ({} meshes, {} materials)", asset->GetPath(),
                 static_cast<uint32_t>(asset->m_PendingData.meshes.size()),
                 static_cast<uint32_t>(asset->m_PendingData.materials.size()));

    Model newModel;
    newModel.Materials.resize(asset->m_PendingData.materials.empty() ? 1 : asset->m_PendingData.materials.size());
    asset->m_EmbeddedTextures.clear();
    auto project = Project::GetActive();

    // Pre-populate embedded textures
    for (auto& [path, embedded] : asset->m_PendingData.embeddedTextures)
    {
        if (embedded.data.empty() || embedded.width <= 0 || embedded.height <= 0 || embedded.isHDR)
        {
            continue;
        }

        auto texture = Texture::Create((uint32_t)embedded.width, (uint32_t)embedded.height, TextureFormat::RGBA8);
        if (texture)
        {
            texture->SetData((void*)embedded.data.data(), 0);
            asset->m_EmbeddedTextures[path] = texture;
        }
    }

    auto loadTex = [&](int matIdx, const std::string& path, int mapIndex) {
        if (path.empty()) return;

        uint32_t texId = 0;
        if (path.front() == '*')
        {
            auto it = asset->m_EmbeddedTextures.find(path);
            if (it != asset->m_EmbeddedTextures.end())
            {
                texId = it->second->GetRendererID();
            }
        }
        else if (project)
        {
            auto tex = AssetManager::Get().Get<TextureAsset>(path);
            if (tex && tex->IsReady())
            {
                texId = tex->GetTexture()->GetRendererID();
            }
        }

        if (texId == 0) return;

        switch (mapIndex)
        {
        case 0: newModel.Materials[matIdx].AlbedoMap = texId; break;
        case 1: newModel.Materials[matIdx].EmissiveMap = texId; break;
        case 2: newModel.Materials[matIdx].NormalMap = texId; break;
        case 3: newModel.Materials[matIdx].MetallicRoughnessMap = texId; break;
        case 4: newModel.Materials[matIdx].EmissiveMap = texId; break;
        case 5: newModel.Materials[matIdx].OcclusionMap = texId; break;
        }
    };

    for (int materialIndex = 0; materialIndex < (int)newModel.Materials.size(); ++materialIndex)
    {
        if (!asset->m_PendingData.materials.empty())
        {
            const auto& rawMaterial = asset->m_PendingData.materials[materialIndex];
            newModel.Materials[materialIndex].AlbedoColor = rawMaterial.albedoColor;
            newModel.Materials[materialIndex].EmissiveColor = rawMaterial.emissiveColor;
            newModel.Materials[materialIndex].EmissiveIntensity = rawMaterial.emissiveIntensity;
            newModel.Materials[materialIndex].Metalness = rawMaterial.metalness;
            newModel.Materials[materialIndex].Roughness = rawMaterial.roughness;
            newModel.Materials[materialIndex].Transparent = rawMaterial.transparent;
            newModel.Materials[materialIndex].Alpha = rawMaterial.albedoColor.a;

            loadTex(materialIndex, rawMaterial.albedoPath, 0);
            newModel.Materials[materialIndex].AlbedoPath = rawMaterial.albedoPath;
            loadTex(materialIndex, rawMaterial.normalPath, 2);
            newModel.Materials[materialIndex].NormalPath = rawMaterial.normalPath;
            loadTex(materialIndex, rawMaterial.occlusionPath, 5);
            newModel.Materials[materialIndex].OcclusionPath = rawMaterial.occlusionPath;
            loadTex(materialIndex, rawMaterial.emissivePath, 4);
            newModel.Materials[materialIndex].EmissivePath = rawMaterial.emissivePath;
            loadTex(materialIndex, rawMaterial.metallicRoughnessPath, 3);
            newModel.Materials[materialIndex].MetallicRoughnessPath = rawMaterial.metallicRoughnessPath;
        }
    }

    for (int meshIndex = 0; meshIndex < (int)asset->m_PendingData.meshes.size(); ++meshIndex)
    {
        const auto& rawMesh = asset->m_PendingData.meshes[meshIndex];
        Mesh mesh;
        mesh.VertexCount = (uint32_t)rawMesh.vertices.size() / 3;
        mesh.TriangleCount = (uint32_t)rawMesh.indices.size() / 3;
        mesh.MaterialIndex = (rawMesh.materialIndex >= 0 && rawMesh.materialIndex < (int)newModel.Materials.size())
                                 ? rawMesh.materialIndex
                                 : 0;

        if (mesh.VertexCount > 0)
        {
            mesh.VAO = VertexArray::Create();

            auto vboPos =
                VertexBuffer::Create(rawMesh.vertices.data(), (uint32_t)rawMesh.vertices.size() * sizeof(float));
            vboPos->SetLayout({{ShaderDataType::Float3, "a_Position"}});
            mesh.VAO->AddVertexBuffer(vboPos);

            if (!rawMesh.texcoords.empty())
            {
                auto vboTex =
                    VertexBuffer::Create(rawMesh.texcoords.data(), (uint32_t)rawMesh.texcoords.size() * sizeof(float));
                vboTex->SetLayout({{ShaderDataType::Float2, "a_TexCoord"}});
                mesh.VAO->AddVertexBuffer(vboTex);
            }

            if (!rawMesh.normals.empty())
            {
                auto vboNorm =
                    VertexBuffer::Create(rawMesh.normals.data(), (uint32_t)rawMesh.normals.size() * sizeof(float));
                vboNorm->SetLayout({{ShaderDataType::Float3, "a_Normal"}});
                mesh.VAO->AddVertexBuffer(vboNorm);
            }

            if (!rawMesh.joints.empty())
            {
                std::vector<int32_t> jointsInt;
                jointsInt.reserve(rawMesh.joints.size());
                for (auto jointId : rawMesh.joints) jointsInt.push_back(static_cast<int32_t>(jointId));
                
                auto vboJoints = VertexBuffer::Create((float*)jointsInt.data(),
                                                      (uint32_t)jointsInt.size() * sizeof(int32_t));
                vboJoints->SetLayout({{ShaderDataType::Int4, "a_JointIDs"}});
                mesh.VAO->AddVertexBuffer(vboJoints);
            }

            if (!rawMesh.weights.empty())
            {
                auto vboWeights = VertexBuffer::Create(rawMesh.weights.data(),
                                                       (uint32_t)rawMesh.weights.size() * sizeof(float));
                vboWeights->SetLayout({{ShaderDataType::Float4, "a_Weights"}});
                mesh.VAO->AddVertexBuffer(vboWeights);
            }

            if (!rawMesh.indices.empty())
            {
                auto ibo = IndexBuffer::Create(rawMesh.indices.data(), (uint32_t)rawMesh.indices.size());
                mesh.VAO->SetIndexBuffer(ibo);
            }

            mesh.MinBounds = rawMesh.MinBounds;
            mesh.MaxBounds = rawMesh.MaxBounds;
        }
        newModel.Meshes.push_back(mesh);
    }

    BoundingBox totalBox = {{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
    bool anyMesh = false;
    for (const auto& inst : asset->m_PendingData.instances)
    {
        if (inst.meshIndex < 0 || inst.meshIndex >= (int)newModel.Meshes.size()) continue;
        const Mesh& mesh = newModel.Meshes[inst.meshIndex];

        glm::vec3 corners[8] = {{mesh.MinBounds.x, mesh.MinBounds.y, mesh.MinBounds.z},
                                {mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MinBounds.z},
                                {mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z},
                                {mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z},
                                {mesh.MinBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z},
                                {mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z},
                                {mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z},
                                {mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z}};

        for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
        {
            glm::vec4 transformed = inst.localTransform * glm::vec4(corners[cornerIndex], 1.0f);
            totalBox.Min = glm::min(totalBox.Min, glm::vec3(transformed));
            totalBox.Max = glm::max(totalBox.Max, glm::vec3(transformed));
        }
        anyMesh = true;
    }
    if (!anyMesh) totalBox = {{0, 0, 0}, {0, 0, 0}};

    asset->m_Model = std::move(newModel);
    asset->m_Materials = asset->m_Model.Materials;
    asset->m_BoundingBox = totalBox;
    asset->m_RawMeshes = std::move(asset->m_PendingData.meshes);
    asset->m_Animations = std::move(asset->m_PendingData.animations);
    asset->m_Instances = std::move(asset->m_PendingData.instances);
    asset->m_OffsetMatrices = std::move(asset->m_PendingData.offsetMatrices);
    asset->m_NodeNames = std::move(asset->m_PendingData.nodeNames);
    asset->m_NodeParents = std::move(asset->m_PendingData.nodeParents);

    asset->m_PendingData = PendingModelData();
    asset->m_HasPendingData = false;
    asset->SetState(AssetState::Ready);
}
} // namespace CHEngine
