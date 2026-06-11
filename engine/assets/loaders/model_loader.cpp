#include "engine/graphics/loaders/model_loader.h"
#include "engine/graphics/loaders/assimp_importer.h"
#include "engine/graphics/loaders/fast_cache.h"
#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include "engine/graphics/managers/texture_manager.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/project/project.h"
#include "engine/core/thread_pool.h"
#include "engine/core/service_registry.h"
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <memory_resource>
#include <string>
#include <filesystem>

namespace CHEngine
{
namespace ModelLoader
{
    namespace
    {
        PendingModelData LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS = 30, class ThreadPool* threadPool = nullptr);
    }


bool Load(std::shared_ptr<Asset> asset, const LoadContext& ctx, std::string* outError)
{
    auto modelAsset = std::dynamic_pointer_cast<ModelAsset>(asset);
    if (!modelAsset)
    {
        if (outError) *outError = "ModelLoader: Invalid asset type";
        return false;
    }

    // Handle procedural models
    if (ctx.ResolvedPath.starts_with(":"))
    {
        if (outError)
        {
            *outError = "ModelLoader: procedural model paths are not supported: " + ctx.ResolvedPath;
        }
        return false;
    }

    auto tp = ctx.Services ? ctx.Services->Get<ThreadPool>() : nullptr;
    auto pendingData = LoadMeshDataFromDisk(ctx.ResolvedPath, 30, tp);
    if (pendingData.isValid)
    {
        modelAsset->SetPendingData(std::move(pendingData));
        return true;
    }
    if (outError)
    {
        *outError = "ModelLoader: failed to import model data from '" + ctx.ResolvedPath + "'";
    }
    return false;
}

    namespace
    {
PendingModelData LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS, ThreadPool* threadPool)
{
    CH_PROFILE_FUNCTION();

    std::filesystem::path cachePath = path;
    cachePath.replace_extension(".chcache");

    PendingModelData data;
    if (std::filesystem::exists(cachePath))
    {
        if (FastCache::Load(cachePath, data))
        {
            CH_CORE_INFO("ModelLoader: Loaded cached model data for '{0}'", path.string());
            return data;
        }
    }

    // Direct import from disk
    data = AssimpImporter::Import(path, samplingFPS, threadPool);
    if (data.isValid)
    {
        CH_CORE_INFO("ModelLoader: Saving model cache for '{0}'", path.string());
        FastCache::Save(cachePath, data);
    }
    return data;
}

    }

bool Finalize(std::shared_ptr<Asset> baseAsset, class AssetManager* assets, std::chrono::steady_clock::time_point budgetEnd)
{
    CH_PROFILE_FUNCTION();
    auto asset = std::dynamic_pointer_cast<ModelAsset>(baseAsset);
    if (!asset || !asset->m_HasPendingData) return true;

    auto& pending = asset->m_PendingData;

    // Phase 1: Initialize textures and materials
    if (pending.FinalizationProgress == 0)
    {
        CH_PROFILE_SCOPE("ModelLoader::Finalize_Init");
        
        // 1. Finalize Embedded Textures (Main thread)
        for (auto& [path, embedded] : pending.embeddedTextures)
        {
            if (asset->m_EmbeddedTextures.find(path) == asset->m_EmbeddedTextures.end())
            {
                auto texture = Texture::Create((uint32_t)embedded.width, (uint32_t)embedded.height, TextureFormat::RGBA8);
                if (texture)
                {
                    texture->SetData((void*)embedded.data.data(), 0);
                    asset->m_EmbeddedTextures.emplace(path, texture);
                }
            }
        }
        pending.embeddedTextures.clear(); 

        // 2. Setup Materials
        asset->m_Model.Materials.clear();
        asset->m_Materials.clear();
        
        for (int i = 0; i < (int)pending.materials.size(); ++i)
        {
            const auto& raw = pending.materials[i];
            Material mat;
            mat.AlbedoColor = raw.albedoColor;
            mat.EmissiveColor = raw.emissiveColor;
            mat.EmissiveIntensity = raw.emissiveIntensity;
            mat.Metalness = raw.metalness;
            mat.Roughness = raw.roughness;
            mat.Transparent = raw.transparent;
            mat.Alpha = raw.albedoColor.a;
            
            auto resolveHandleAndTriggerLoad = [&](const std::pmr::string& p) -> AssetHandle {
                if (p.empty()) return AssetHandle(0);
                if (p[0] == '*') return AssetHandle(0); 
                return assets ? assets->ResolveToHandle(std::string(p.c_str()), AssetType::Texture) : AssetHandle(0);
            };

            auto resolveEmbeddedTex = [&](const std::pmr::string& p) -> uint32_t {
                if (p.empty() || p[0] != '*') return 0;
                auto it = asset->m_EmbeddedTextures.find(p);
                if (it == asset->m_EmbeddedTextures.end()) return 0;
                return it->second ? it->second->GetRendererID() : 0;
            };

            mat.AlbedoMap = resolveEmbeddedTex(raw.albedoPath);
            mat.NormalMap = resolveEmbeddedTex(raw.normalPath);
            mat.MetallicRoughnessMap = resolveEmbeddedTex(raw.metallicRoughnessPath);
            mat.EmissiveMap = resolveEmbeddedTex(raw.emissivePath);
            mat.OcclusionMap = resolveEmbeddedTex(raw.occlusionPath);

            mat.AlbedoHandle = resolveHandleAndTriggerLoad(raw.albedoPath);
            mat.NormalHandle = resolveHandleAndTriggerLoad(raw.normalPath);
            mat.MetallicRoughnessHandle = resolveHandleAndTriggerLoad(raw.metallicRoughnessPath);
            mat.EmissiveHandle = resolveHandleAndTriggerLoad(raw.emissivePath);
            mat.OcclusionHandle = resolveHandleAndTriggerLoad(raw.occlusionPath);

            asset->m_Model.Materials.push_back(mat);
            asset->m_Materials.push_back(mat);
        }

        if (asset->m_Model.Materials.empty())
        {
            asset->m_Model.Materials.emplace_back();
            asset->m_Materials.emplace_back();
        }
    }

    // Phase 2: Process ALL Meshes
    while (pending.FinalizationProgress < (int)pending.meshes.size())
    {
        CH_PROFILE_SCOPE("ModelLoader::Finalize_Mesh");
        const auto& rawMesh = pending.meshes[pending.FinalizationProgress];
        Mesh mesh;
        mesh.VertexCount = (uint32_t)rawMesh.vertices.size() / 3;
        mesh.TriangleCount = (uint32_t)rawMesh.indices.size() / 3;
        mesh.MaterialIndex = (rawMesh.materialIndex >= 0 && rawMesh.materialIndex < (int)asset->m_Materials.size()) 
                                 ? rawMesh.materialIndex : 0;
        mesh.MinBounds = rawMesh.MinBounds;
        mesh.MaxBounds = rawMesh.MaxBounds;

        if (mesh.VertexCount > 0)
        {
            mesh.VAO = VertexArray::Create();

            auto vboPos = VertexBuffer::Create(rawMesh.vertices.data(), (uint32_t)rawMesh.vertices.size() * sizeof(float));
            vboPos->SetLayout({{ShaderDataType::Float3, "a_Position"}});
            mesh.VAO->AddVertexBuffer(vboPos);

            if (!rawMesh.texcoords.empty())
            {
                auto vboTex = VertexBuffer::Create(rawMesh.texcoords.data(), (uint32_t)rawMesh.texcoords.size() * sizeof(float));
                vboTex->SetLayout({{ShaderDataType::Float2, "a_TexCoord"}});
                mesh.VAO->AddVertexBuffer(vboTex);
            }

            if (!rawMesh.normals.empty())
            {
                auto vboNorm = VertexBuffer::Create(rawMesh.normals.data(), (uint32_t)rawMesh.normals.size() * sizeof(float));
                vboNorm->SetLayout({{ShaderDataType::Float3, "a_Normal"}});
                mesh.VAO->AddVertexBuffer(vboNorm);
            }

            if (!rawMesh.tangents.empty())
            {
                auto vboTang = VertexBuffer::Create(rawMesh.tangents.data(), (uint32_t)rawMesh.tangents.size() * sizeof(float));
                vboTang->SetLayout({{ShaderDataType::Float3, "a_Tangent"}});
                mesh.VAO->AddVertexBuffer(vboTang);
            }

            if (!rawMesh.joints.empty())
            {
                std::vector<int32_t> jointsInt;
                jointsInt.reserve(rawMesh.joints.size());
                for (auto jointId : rawMesh.joints) jointsInt.push_back(static_cast<int32_t>(jointId));
                
                auto vboJoints = VertexBuffer::Create((float*)jointsInt.data(), (uint32_t)jointsInt.size() * sizeof(int32_t));
                vboJoints->SetLayout({{ShaderDataType::Int4, "a_JointIDs"}});
                mesh.VAO->AddVertexBuffer(vboJoints);
            }

            if (!rawMesh.weights.empty())
            {
                auto vboWeights = VertexBuffer::Create(rawMesh.weights.data(), (uint32_t)rawMesh.weights.size() * sizeof(float));
                vboWeights->SetLayout({{ShaderDataType::Float4, "a_Weights"}});
                mesh.VAO->AddVertexBuffer(vboWeights);
            }

            if (!rawMesh.indices.empty())
            {
                auto ibo = IndexBuffer::Create(rawMesh.indices.data(), (uint32_t)rawMesh.indices.size());
                mesh.VAO->SetIndexBuffer(ibo);
            }
        }

        asset->m_Model.Meshes.push_back(mesh);
        pending.FinalizationProgress++;

        if (std::chrono::steady_clock::now() >= budgetEnd)
        {
            return false;
        }
    }

    // Phase 3: Post-processing
    if (pending.FinalizationProgress >= (int)pending.meshes.size())
    {
        CH_PROFILE_SCOPE("ModelLoader::Finalize_Post");
        
        BoundingBox totalBox = {{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
        bool anyInstance = false;
        for (const auto& inst : pending.instances)
        {
            if (inst.meshIndex < 0 || inst.meshIndex >= (int)asset->m_Model.Meshes.size()) continue;
            const auto& mesh = asset->m_Model.Meshes[inst.meshIndex];

            glm::vec3 corners[8] = {
                {mesh.MinBounds.x, mesh.MinBounds.y, mesh.MinBounds.z},
                {mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MinBounds.z},
                {mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z},
                {mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z},
                {mesh.MinBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z},
                {mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z},
                {mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z},
                {mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z}
            };

            for (int k = 0; k < 8; ++k)
            {
                glm::vec4 transformed = inst.localTransform * glm::vec4(corners[k], 1.0f);
                totalBox.Min = glm::min(totalBox.Min, glm::vec3(transformed));
                totalBox.Max = glm::max(totalBox.Max, glm::vec3(transformed));
            }
            anyInstance = true;
        }
        if (!anyInstance) totalBox = {{0, 0, 0}, {0, 0, 0}};

        asset->m_BoundingBox = totalBox;
        asset->m_RawMeshes = std::move(pending.meshes);
        asset->m_Animations = std::move(pending.animations);
        asset->m_Instances = std::move(pending.instances);
        asset->m_OffsetMatrices = std::move(pending.offsetMatrices);
        asset->m_NodeNames = std::move(pending.nodeNames);
        asset->m_NodeParents = std::move(pending.nodeParents);

        asset->m_HasPendingData = false;
        asset->SetState(AssetState::Ready);
        return true; 
    }

    return false;
}
} // namespace ModelLoader
} // namespace CHEngine
