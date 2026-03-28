#include "engine/graphics/assets/model_asset.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "engine/graphics/importers/mesh_importer.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/assets/texture_asset.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace CHEngine
{
void ModelAsset::UploadToGPU()
{
    if (!m_HasPendingData || !m_PendingData.isValid)
    {
        return;
    }

    CH_CORE_INFO("ModelAsset: Uploading model to GPU: '{}' ({} meshes, {} materials)", GetPath(),
                 m_PendingData.meshes.size(), m_PendingData.materials.size());

    Model newModel;
    newModel.Materials.resize(m_PendingData.materials.empty() ? 1 : m_PendingData.materials.size());

    // Load Materials
    auto project = Project::GetActive();
    std::vector<std::shared_ptr<TextureAsset>> localTextures;
    std::vector<PendingTexture> localPendingTextures;

    auto loadTex = [&](int matIdx, const std::string& path, int mapIndex) {
        if (path.empty() || !project) return;
        auto tex = AssetManager::Get().Get<TextureAsset>(path);
        if (!tex) return;
        if (tex->IsReady()) {
            uint32_t texId = tex->GetTexture().id;
            if (mapIndex == 0) newModel.Materials[matIdx].AlbedoMap = texId;
            else if (mapIndex == 2) newModel.Materials[matIdx].NormalMap = texId;
            else if (mapIndex == 3) newModel.Materials[matIdx].MetallicRoughnessMap = texId;
            else if (mapIndex == 4) newModel.Materials[matIdx].EmissiveMap = texId;
            else if (mapIndex == 5) newModel.Materials[matIdx].OcclusionMap = texId;
            localTextures.push_back(tex);
        } else {
            localPendingTextures.push_back({matIdx, path, mapIndex});
        }
    };

    for (int i = 0; i < (int)newModel.Materials.size(); ++i)
    {
        if (!m_PendingData.materials.empty())
        {
            const auto& rawMaterial = m_PendingData.materials[i];
            newModel.Materials[i].AlbedoColor = rawMaterial.albedoColor;
            newModel.Materials[i].EmissiveColor = rawMaterial.emissiveColor;
            newModel.Materials[i].EmissiveIntensity = rawMaterial.emissiveIntensity;
            newModel.Materials[i].Metalness = rawMaterial.metalness;
            newModel.Materials[i].Roughness = rawMaterial.roughness;

            loadTex(i, rawMaterial.albedoPath, 0); // Albedo
            loadTex(i, rawMaterial.emissivePath, 1); // Emissive
            loadTex(i, rawMaterial.normalPath, 2); // Normal
            loadTex(i, rawMaterial.occlusionPath, 4); // Occlusion
            loadTex(i, rawMaterial.metallicRoughnessPath, 3); // MetallicRoughness
        }
    }

    // Load Meshes
    for (int i = 0; i < (int)m_PendingData.meshes.size(); ++i)
    {
        const auto& rawMesh = m_PendingData.meshes[i];
        Mesh mesh;
        mesh.VertexCount = (uint32_t)rawMesh.vertices.size() / 3;
        mesh.TriangleCount = (uint32_t)rawMesh.indices.size() / 3;
        mesh.MaterialIndex = (rawMesh.materialIndex >= 0 && rawMesh.materialIndex < (int)newModel.Materials.size()) ? rawMesh.materialIndex : 0;

        if (mesh.VertexCount > 0)
        {
            mesh.VAO = VertexArray::Create();

            // Positions
            auto vboPos = VertexBuffer::Create(rawMesh.vertices.data(), (uint32_t)rawMesh.vertices.size() * sizeof(float));
            vboPos->SetLayout({ { ShaderDataType::Float3, "a_Position" } });
            mesh.VAO->AddVertexBuffer(vboPos);

            // Texcoords
            if (!rawMesh.texcoords.empty())
            {
                auto vboTex = VertexBuffer::Create(rawMesh.texcoords.data(), (uint32_t)rawMesh.texcoords.size() * sizeof(float));
                vboTex->SetLayout({ { ShaderDataType::Float2, "a_TexCoord" } });
                mesh.VAO->AddVertexBuffer(vboTex);
            }

            // Normals
            if (!rawMesh.normals.empty())
            {
                auto vboNorm = VertexBuffer::Create(rawMesh.normals.data(), (uint32_t)rawMesh.normals.size() * sizeof(float));
                vboNorm->SetLayout({ { ShaderDataType::Float3, "a_Normal" } });
                mesh.VAO->AddVertexBuffer(vboNorm);
            }

            // Indices
            if (!rawMesh.indices.empty())
            {
                // Convert uint16_t to uint32_t if needed, but our IndexBuffer::Create usually handles uint32_t or uint16_t
                // Let's assume our API supports uint16_t or we convert here.
                std::vector<uint32_t> indices32(rawMesh.indices.begin(), rawMesh.indices.end());
                auto ibo = IndexBuffer::Create(indices32.data(), (uint32_t)indices32.size());
                mesh.VAO->SetIndexBuffer(ibo);
            }

            // Calculate local bounds
            mesh.MinBounds = { FLT_MAX, FLT_MAX, FLT_MAX };
            mesh.MaxBounds = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
            for (size_t v = 0; v < rawMesh.vertices.size(); v += 3)
            {
                mesh.MinBounds.x = std::min(mesh.MinBounds.x, rawMesh.vertices[v]);
                mesh.MinBounds.y = std::min(mesh.MinBounds.y, rawMesh.vertices[v+1]);
                mesh.MinBounds.z = std::min(mesh.MinBounds.z, rawMesh.vertices[v+2]);
                mesh.MaxBounds.x = std::max(mesh.MaxBounds.x, rawMesh.vertices[v]);
                mesh.MaxBounds.y = std::max(mesh.MaxBounds.y, rawMesh.vertices[v+1]);
                mesh.MaxBounds.z = std::max(mesh.MaxBounds.z, rawMesh.vertices[v+2]);
            }
        }

        newModel.Meshes.push_back(mesh);
    }

    // Calculate total bounding box using instances
    BoundingBox totalBox = { {FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX} };
    bool anyMesh = false;
    for (const auto& inst : m_PendingData.instances)
    {
        if (inst.meshIndex < 0 || inst.meshIndex >= (int)newModel.Meshes.size()) continue;
        const Mesh& mesh = newModel.Meshes[inst.meshIndex];
        
        glm::vec3 corners[8] = {
            { mesh.MinBounds.x, mesh.MinBounds.y, mesh.MinBounds.z },
            { mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MinBounds.z },
            { mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z },
            { mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z },
            { mesh.MinBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z },
            { mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z },
            { mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z },
            { mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z }
        };

        for (int c = 0; c < 8; c++)
        {
            glm::vec4 transformed = inst.localTransform * glm::vec4(corners[c], 1.0f);
            totalBox.Min = glm::min(totalBox.Min, glm::vec3(transformed));
            totalBox.Max = glm::max(totalBox.Max, glm::vec3(transformed));
        }
        anyMesh = true;
    }
    if (!anyMesh) totalBox = { {0,0,0}, {0,0,0} };

    // Final transfer
    {
        std::lock_guard<std::mutex> lock(m_ModelMutex);
        m_Model = std::move(newModel);
        m_Textures = std::move(localTextures);
        m_PendingTextures = std::move(localPendingTextures);
        m_BoundingBox = totalBox;
        m_Animations = std::move(m_PendingData.animations);
        m_Instances = std::move(m_PendingData.instances);
        m_OffsetMatrices = std::move(m_PendingData.offsetMatrices);
        m_NodeNames = std::move(m_PendingData.nodeNames);
        m_NodeParents = std::move(m_PendingData.nodeParents);
        m_GlobalNodeTransforms = std::move(m_PendingData.globalBindPoses);
        m_RawMeshes = std::move(m_PendingData.meshes);
    }

    m_PendingData = PendingModelData();
    m_HasPendingData = false;
    SetState(AssetState::Ready);
    CH_CORE_INFO("ModelAsset: GPU upload completed for '{}'", GetPath());
}

ModelAsset::~ModelAsset()
{
}


void ModelAsset::OnUpdate()
{
    if (m_PendingTextures.empty())
    {
        return;
    }

    auto project = Project::GetActive();
    if (!project)
    {
        return;
    }

    auto& assetManager = AssetManager::Get();

    std::lock_guard<std::mutex> lock(m_ModelMutex);

    for (auto it = m_PendingTextures.begin(); it != m_PendingTextures.end();)
    {
        auto textureAsset = assetManager.Get<TextureAsset>(it->path);
        if (textureAsset && textureAsset->IsReady())
        {
            CH_CORE_INFO("ModelAsset: Applying deferred texture '{}' to material {}", it->path, it->materialIndex);

            if (it->materialIndex >= 0 && it->materialIndex < (int)m_Model.Materials.size())
            {
                uint32_t texId = textureAsset->GetTexture().id;
                if (it->mapIndex == 0) m_Model.Materials[it->materialIndex].AlbedoMap = texId;
                else if (it->mapIndex == 2) m_Model.Materials[it->materialIndex].NormalMap = texId;
                else if (it->mapIndex == 3) m_Model.Materials[it->materialIndex].MetallicRoughnessMap = texId;
                else if (it->mapIndex == 4) m_Model.Materials[it->materialIndex].EmissiveMap = texId;
                else if (it->mapIndex == 5) m_Model.Materials[it->materialIndex].OcclusionMap = texId;
                
                m_Textures.push_back(textureAsset);
            }

            it = m_PendingTextures.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
BoundingBox ModelAsset::GetBoundingBox() const
{
    return m_BoundingBox;
}

Model& ModelAsset::GetModel()
{
    std::lock_guard<std::mutex> lock(m_ModelMutex);
    return m_Model;
}

const Model& ModelAsset::GetModel() const
{
    std::lock_guard<std::mutex> lock(m_ModelMutex);
    return m_Model;
}

std::vector<std::shared_ptr<TextureAsset>> ModelAsset::GetTextures() const
{
    std::lock_guard<std::mutex> lock(m_ModelMutex);
    return m_Textures;
}

std::vector<glm::mat4> ModelAsset::ComputeAnimationPose(int animationIndex, float frameIndex, int targetAnimationIndex, float targetFrameIndex, float blendWeight)
{
    std::lock_guard<std::mutex> lock(m_ModelMutex);
    
    // Animation system needs to be refactored to use glm matrices directly
    // For now we return empty as I've removed the legacy Raylib-dependent calculation.
    return {};
}
} // namespace CHEngine
