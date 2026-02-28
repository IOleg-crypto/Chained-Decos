#include "model_asset.h"
#include "asset_manager.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "engine/scene/project.h"
#include "mesh_importer.h"
#include "raylib.h"
#include "raymath.h"
#include "shader_asset.h"
#include "texture_asset.h"
#include <algorithm>
#include <cstring>
#include <filesystem>

namespace CHEngine
{
void ModelAsset::UploadToGPU()
{
    if (!m_HasPendingData || !m_PendingData.isValid)
    {
        return;
    }

    // Convert Raw Data to Raylib objects
    CH_CORE_INFO("ModelAsset: Creating Raylib model for '{}' ({} meshes, {} materials)", GetPath(),
                 m_PendingData.meshes.size(), m_PendingData.materials.size());

    Model model = {0};
    model.meshCount = (int)m_PendingData.meshes.size();
    if (model.meshCount > 0)
    {
        model.meshes = (Mesh*)RL_CALLOC(model.meshCount, sizeof(Mesh));
        model.meshMaterial = (int*)RL_CALLOC(model.meshCount, sizeof(int));
    }

    model.materialCount = (int)(m_PendingData.materials.empty() ? 1 : m_PendingData.materials.size());
    model.materials = (Material*)RL_CALLOC(model.materialCount, sizeof(Material));

    // Load Materials
    auto project = Project::GetActive();
    std::vector<std::shared_ptr<TextureAsset>> localTextures;
    std::vector<PendingTexture> localPendingTextures;

    // Helper: load one texture channel, queue if still loading
    auto loadTex = [&](int matIdx, const std::string& path, int mapIndex) {
        if (path.empty() || !project) return;
        auto tex = project->GetAssetManager()->Get<TextureAsset>(path);
        if (!tex) return;
        if (tex->IsReady()) {
            model.materials[matIdx].maps[mapIndex].texture = tex->GetTexture();
            localTextures.push_back(tex);
        } else {
            localPendingTextures.push_back({matIdx, path, mapIndex});
        }
    };

    for (int i = 0; i < model.materialCount; ++i)
    {
        model.materials[i] = LoadMaterialDefault();

        if (!m_PendingData.materials.empty())
        {
            const auto& rawMaterial = m_PendingData.materials[i];

            model.materials[i].maps[MATERIAL_MAP_ALBEDO].color   = rawMaterial.albedoColor;
            model.materials[i].maps[MATERIAL_MAP_EMISSION].color  = rawMaterial.emissiveColor;
            model.materials[i].maps[MATERIAL_MAP_METALNESS].value = rawMaterial.metalness;
            model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].value = rawMaterial.roughness;

            loadTex(i, rawMaterial.albedoPath,            MATERIAL_MAP_ALBEDO);
            loadTex(i, rawMaterial.emissivePath,          MATERIAL_MAP_EMISSION);
            loadTex(i, rawMaterial.normalPath,            MATERIAL_MAP_NORMAL);
            loadTex(i, rawMaterial.occlusionPath,         MATERIAL_MAP_OCCLUSION);

            // MetallicRoughness is a packed texture shared by two slots
            if (!rawMaterial.metallicRoughnessPath.empty() && project)
            {
                auto tex = project->GetAssetManager()->Get<TextureAsset>(rawMaterial.metallicRoughnessPath);
                if (tex && tex->IsReady()) {
                    model.materials[i].maps[MATERIAL_MAP_METALNESS].texture = tex->GetTexture();
                    model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].texture = tex->GetTexture();
                    localTextures.push_back(tex);
                } else if (tex) {
                    localPendingTextures.push_back({i, rawMaterial.metallicRoughnessPath, MATERIAL_MAP_METALNESS});
                    localPendingTextures.push_back({i, rawMaterial.metallicRoughnessPath, MATERIAL_MAP_ROUGHNESS});
                }
            }
        }
    }

    // Load Meshes
    for (int i = 0; i < model.meshCount; ++i)
    {
        const auto& rawMesh = m_PendingData.meshes[i];
        Mesh mesh = {0};
        mesh.vertexCount = (int)rawMesh.vertices.size() / 3;
        mesh.triangleCount = (int)rawMesh.indices.size() / 3;

        if (mesh.vertexCount > 0)
        {
            mesh.vertices = (float*)RL_MALLOC(rawMesh.vertices.size() * sizeof(float));
            std::memcpy(mesh.vertices, rawMesh.vertices.data(), rawMesh.vertices.size() * sizeof(float));

            if (!rawMesh.texcoords.empty())
            {
                mesh.texcoords = (float*)RL_MALLOC(rawMesh.texcoords.size() * sizeof(float));
                std::memcpy(mesh.texcoords, rawMesh.texcoords.data(), rawMesh.texcoords.size() * sizeof(float));
            }

            if (!rawMesh.normals.empty())
            {
                mesh.normals = (float*)RL_MALLOC(rawMesh.normals.size() * sizeof(float));
                std::memcpy(mesh.normals, rawMesh.normals.data(), rawMesh.normals.size() * sizeof(float));
            }

            if (!rawMesh.indices.empty())
            {
                mesh.indices = (unsigned short*)RL_MALLOC(rawMesh.indices.size() * sizeof(unsigned short));
                std::memcpy(mesh.indices, rawMesh.indices.data(), rawMesh.indices.size() * sizeof(unsigned short));
            }

            if (!rawMesh.colors.empty())
            {
                mesh.colors = (unsigned char*)RL_MALLOC(rawMesh.colors.size() * sizeof(unsigned char));
                std::memcpy(mesh.colors, rawMesh.colors.data(), rawMesh.colors.size() * sizeof(unsigned char));
            }

            if (!rawMesh.tangents.empty())
            {
                mesh.tangents = (float*)RL_MALLOC(rawMesh.tangents.size() * sizeof(float));
                std::memcpy(mesh.tangents, rawMesh.tangents.data(), rawMesh.tangents.size() * sizeof(float));
            }

            if (!rawMesh.joints.empty())
            {
                mesh.boneIds = (unsigned char*)RL_MALLOC(rawMesh.joints.size() * sizeof(unsigned char));
                std::memcpy(mesh.boneIds, rawMesh.joints.data(), rawMesh.joints.size() * sizeof(unsigned char));
            }

            if (!rawMesh.weights.empty())
            {
                mesh.boneWeights = (float*)RL_MALLOC(rawMesh.weights.size() * sizeof(float));
                std::memcpy(mesh.boneWeights, rawMesh.weights.data(), rawMesh.weights.size() * sizeof(float));
            }

            UploadMesh(&mesh, false);

            // If no tangents but we have normals and texcoords, generate them
            if (mesh.tangents == nullptr && mesh.normals != nullptr && mesh.texcoords != nullptr)
            {
                GenMeshTangents(&mesh);
                // Re-upload with tangents
                UpdateMeshBuffer(mesh, 4, mesh.tangents, mesh.vertexCount * 4 * sizeof(float), 0);
            }
        }

        model.meshes[i] = mesh;
        model.meshMaterial[i] =
            (rawMesh.materialIndex >= 0 && rawMesh.materialIndex < model.materialCount) ? rawMesh.materialIndex : 0;
    }

    model.transform = MatrixIdentity();

    // Lock and transfer
    {
        std::lock_guard<std::mutex> lock(m_ModelMutex);

        // Free previous model data to prevent leaks on re-upload
        if (m_Model.meshCount > 0)
        {
            ::UnloadModel(m_Model);
        }

        m_Model = model;
        m_Textures = std::move(localTextures);
        m_PendingTextures = std::move(localPendingTextures);

        // Build skeleton from pending bone data
        if (!m_PendingData.bones.empty())
        {
            m_Model.boneCount = (int)m_PendingData.bones.size();
            m_Model.bones = (BoneInfo*)RL_MALLOC(m_Model.boneCount * sizeof(BoneInfo));
            std::memcpy(m_Model.bones, m_PendingData.bones.data(), m_Model.boneCount * sizeof(BoneInfo));

            m_Model.bindPose = (Transform*)RL_MALLOC(m_Model.boneCount * sizeof(Transform));
            for (int i = 0; i < m_Model.boneCount; i++)
            {
                Matrix mat = m_PendingData.nodeLocalTransforms[i];
                m_Model.bindPose[i].translation = {mat.m12, mat.m13, mat.m14};
                m_Model.bindPose[i].rotation = QuaternionFromMatrix(mat);
                m_Model.bindPose[i].scale = {Vector3Length({mat.m0, mat.m1, mat.m2}),
                                             Vector3Length({mat.m4, mat.m5, mat.m6}),
                                             Vector3Length({mat.m8, mat.m9, mat.m10})};
            }
        }

        // Transfer runtime data
        m_Animations = std::move(m_PendingData.animations);

        // Transfer hierarchy data
        m_OffsetMatrices       = std::move(m_PendingData.offsetMatrices);
        m_NodeNames            = std::move(m_PendingData.nodeNames);
        m_NodeParents          = std::move(m_PendingData.nodeParents);
        m_MeshToNode           = std::move(m_PendingData.meshToNode);
        m_GlobalNodeTransforms = std::move(m_PendingData.globalBindPoses);
    }

    m_PendingData = PendingModelData();
    m_HasPendingData = false;

    SetState(AssetState::Ready);
    CH_CORE_INFO("ModelAsset: GPU upload completed for '{}'", GetPath());
}

ModelAsset::~ModelAsset()
{
    if (m_Model.meshCount > 0)
    {
        ::UnloadModel(m_Model);
    }
    // m_Animations is std::vector, no need for UnloadModelAnimations
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

    auto assetManager = project->GetAssetManager();

    std::lock_guard<std::mutex> lock(m_ModelMutex);

    for (auto it = m_PendingTextures.begin(); it != m_PendingTextures.end();)
    {
        auto textureAsset = assetManager->Get<TextureAsset>(it->path);
        if (textureAsset && textureAsset->IsReady())
        {
            CH_CORE_INFO("ModelAsset: Applying deferred texture '{}' to material {}", it->path, it->materialIndex);

            if (it->materialIndex >= 0 && it->materialIndex < m_Model.materialCount)
            {
                m_Model.materials[it->materialIndex].maps[it->mapIndex].texture = textureAsset->GetTexture();
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
    return ::GetModelBoundingBox(m_Model);
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

} // namespace CHEngine
