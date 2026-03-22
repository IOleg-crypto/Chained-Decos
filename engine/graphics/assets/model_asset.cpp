#include "engine/graphics/assets/model_asset.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "engine/graphics/importers/mesh_importer.h"
#include "raylib.h"
#include "raymath.h"
#include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/assets/texture_asset.h"
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
    model.transform = MatrixIdentity();
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
        auto tex = AssetManager::Get().Get<TextureAsset>(path);
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
                auto tex = AssetManager::Get().Get<TextureAsset>(rawMaterial.metallicRoughnessPath);
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

    // Lock and transfer
    {
        std::lock_guard<std::mutex> lock(m_ModelMutex);

        // Free previous model data to prevent leaks on re-upload
        if (m_Model.meshCount > 0)
        {
            ::UnloadModel(m_Model);
        }

        m_Model = model;
        m_Model.transform = MatrixIdentity(); // Already baked into instances
        m_Textures = std::move(localTextures);
        m_PendingTextures = std::move(localPendingTextures);

        // Cache the bounding box once (expensive calculation)
        // We calculate a hierarchy-aware bounding box using the flat instance list
        BoundingBox totalBox = {{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
        bool anyMesh = false;

        for (const auto& inst : m_PendingData.instances)
        {
            if (inst.meshIndex < 0 || inst.meshIndex >= model.meshCount) continue;
            Mesh& mesh = model.meshes[inst.meshIndex];
            if (mesh.vertexCount == 0) continue;

            // Correct order: local * root (root is now identity for consistency)
            Matrix transform = inst.localTransform;

            BoundingBox meshBox = ::GetMeshBoundingBox(mesh);
            
            // Transform all 8 corners of the mesh AABB to model space
            Vector3 corners[8] = {
                { meshBox.min.x, meshBox.min.y, meshBox.min.z },
                { meshBox.max.x, meshBox.min.y, meshBox.min.z },
                { meshBox.min.x, meshBox.max.y, meshBox.min.z },
                { meshBox.max.x, meshBox.max.y, meshBox.min.z },
                { meshBox.min.x, meshBox.min.y, meshBox.max.z },
                { meshBox.max.x, meshBox.min.y, meshBox.max.z },
                { meshBox.min.x, meshBox.max.y, meshBox.max.z },
                { meshBox.max.x, meshBox.max.y, meshBox.max.z }
            };

            for (int c = 0; c < 8; c++)
            {
                Vector3 transformed = Vector3Transform(corners[c], transform);
                totalBox.min = Vector3Min(totalBox.min, transformed);
                totalBox.max = Vector3Max(totalBox.max, transformed);
            }
            
            anyMesh = true;
        }
        
        if (!anyMesh) totalBox = {{0,0,0}, {0,0,0}};
        m_BoundingBox = totalBox;

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
        m_Instances            = std::move(m_PendingData.instances);
        m_OffsetMatrices       = std::move(m_PendingData.offsetMatrices);
        m_NodeNames            = std::move(m_PendingData.nodeNames);
        m_NodeParents          = std::move(m_PendingData.nodeParents);
        m_GlobalNodeTransforms = std::move(m_PendingData.globalBindPoses);

        // De-overengineering: Clear skeletal data for static models
        // We do this after transfer to ensure they are truly purged for static objects
        if (m_Animations.empty())
        {
            m_OffsetMatrices.clear();
            m_GlobalNodeTransforms.clear();
            m_NodeParents.clear();
            m_NodeNames.clear();
            // Note: m_Model.boneCount and bone/pose pointers were already set above,
            // we should null them out to prevent accidental access if static.
            m_Model.boneCount = 0;
            // Raylib allocated these via RL_MALLOC if m_PendingData.bones wasn't empty.
            // But if it's static, was m_PendingData.bones empty?
            // Usually yes, if it's a truly static mesh.
        }
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

    auto& assetManager = AssetManager::Get();

    std::lock_guard<std::mutex> lock(m_ModelMutex);

    for (auto it = m_PendingTextures.begin(); it != m_PendingTextures.end();)
    {
        auto textureAsset = assetManager.Get<TextureAsset>(it->path);
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

std::vector<Matrix> ModelAsset::ComputeAnimationPose(int animationIndex, float frameIndex, int targetAnimationIndex, float targetFrameIndex, float blendWeight)
{
    std::lock_guard<std::mutex> lock(m_ModelMutex);
    
    if (m_Model.boneCount <= 0 || m_OffsetMatrices.empty()) return {};

    int boneCount = m_Model.boneCount;
    std::vector<Matrix> boneMatrices(boneCount);
    std::vector<Matrix> globalPose(boneCount);
    std::vector<Transform> localPoseA(boneCount);

    auto CalculateLocalPose = [&](int animIdx, float fIdx, std::vector<Transform>& outLocalPose) {
        if (animIdx >= 0 && animIdx < (int)m_Animations.size())
        {
            const auto& anim = m_Animations[animIdx];
            int currentFrame = (int)fIdx % anim.frameCount;
            int nextFrame = (currentFrame + 1) % anim.frameCount;
            float interp = fIdx - (float)((int)fIdx);

            for (int i = 0; i < anim.boneCount; i++)
            {
                Transform t = anim.framePoses[currentFrame * anim.boneCount + i];
                Transform tNext = anim.framePoses[nextFrame * anim.boneCount + i];

                outLocalPose[i].translation = Vector3Lerp(t.translation, tNext.translation, interp);
                outLocalPose[i].rotation = QuaternionSlerp(t.rotation, tNext.rotation, interp);
                outLocalPose[i].scale = Vector3Lerp(t.scale, tNext.scale, interp);
            }
            return true;
        }
        return false;
    };

    if (!CalculateLocalPose(animationIndex, frameIndex, localPoseA))
    {
        for (int i = 0; i < boneCount; i++) localPoseA[i] = m_Model.bindPose[i];
    }

    if (targetAnimationIndex >= 0 && blendWeight > 0.0f)
    {
        std::vector<Transform> localPoseB(boneCount);
        if (CalculateLocalPose(targetAnimationIndex, targetFrameIndex, localPoseB))
        {
            for (int i = 0; i < boneCount; i++)
            {
                localPoseA[i].translation = Vector3Lerp(localPoseA[i].translation, localPoseB[i].translation, blendWeight);
                localPoseA[i].rotation = QuaternionSlerp(localPoseA[i].rotation, localPoseB[i].rotation, blendWeight);
                localPoseA[i].scale = Vector3Lerp(localPoseA[i].scale, localPoseB[i].scale, blendWeight);
            }
        }
    }

    // Convert to global and then to bone matrices
    for (int i = 0; i < boneCount; i++)
    {
        Matrix localMat = MatrixMultiply(QuaternionToMatrix(localPoseA[i].rotation), 
                                         MatrixTranslate(localPoseA[i].translation.x, localPoseA[i].translation.y, localPoseA[i].translation.z));
        localMat = MatrixMultiply(MatrixScale(localPoseA[i].scale.x, localPoseA[i].scale.y, localPoseA[i].scale.z), localMat);

        int parent = m_Model.bones[i].parent;
        // Global = Local * ParentGlobal (Raylib math v * M)
        globalPose[i] = (parent == -1) ? localMat : MatrixMultiply(globalPose[parent], localMat);
        boneMatrices[i] = MatrixMultiply(m_OffsetMatrices[i], globalPose[i]);
    }

    return boneMatrices;
}
} // namespace CHEngine
