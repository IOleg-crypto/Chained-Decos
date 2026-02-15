#include "model_asset.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "raylib.h"
#include "shader_asset.h"
#include "texture_asset.h"
#include <filesystem>
#include <algorithm>
#include <cstring>
#include "mesh_importer.h"
#include "engine/scene/project.h"
#include "asset_manager.h"

namespace CHEngine
{
    void ModelAsset::UploadToGPU()
    {
        if (!m_HasPendingData || !m_PendingData.isValid)
            return;
        
        // Convert Raw Data to Raylib objects
        CH_CORE_INFO("ModelAsset: Creating Raylib model for '{}' ({} meshes, {} materials)", GetPath(), m_PendingData.meshes.size(), m_PendingData.materials.size());
        
        Model model = {0};
        model.meshCount = (int)m_PendingData.meshes.size();
        if (model.meshCount > 0) {
            model.meshes = (Mesh*)RL_CALLOC(model.meshCount, sizeof(Mesh));
            model.meshMaterial = (int*)RL_CALLOC(model.meshCount, sizeof(int));
        }
        
        model.materialCount = (int)(m_PendingData.materials.empty() ? 1 : m_PendingData.materials.size());
        model.materials = (Material*)RL_CALLOC(model.materialCount, sizeof(Material));
        
        // Load Materials
        auto project = Project::GetActive();
        m_PendingTextures.clear(); // Clear any old pending ones

        for (int i = 0; i < model.materialCount; ++i) {
            model.materials[i] = LoadMaterialDefault();
            
            if (!m_PendingData.materials.empty()) {
                const auto& rawMaterial = m_PendingData.materials[i];
                model.materials[i].maps[MATERIAL_MAP_ALBEDO].color = rawMaterial.albedoColor;
                
                if (!rawMaterial.albedoPath.empty() && project) {
                    auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(rawMaterial.albedoPath);
                    if (textureAsset && textureAsset->IsReady()) {
                        model.materials[i].maps[MATERIAL_MAP_ALBEDO].texture = textureAsset->GetTexture();
                        m_Textures.push_back(textureAsset);
                        CH_CORE_INFO("ModelAsset: Loaded texture '{}' for material {}", rawMaterial.albedoPath, i);
                    }
                    else if (textureAsset) {
                        CH_CORE_WARN("ModelAsset: Texture '{}' exists but not ready (state: {}). Adding to pending.", 
                            rawMaterial.albedoPath, (int)textureAsset->GetState());
                        m_PendingTextures.push_back({i, rawMaterial.albedoPath, MATERIAL_MAP_ALBEDO});
                    }
                    else {
                        CH_CORE_WARN("ModelAsset: Failed to load texture '{}'", rawMaterial.albedoPath);
                    }
                }

                // Handle Emissive
                model.materials[i].maps[MATERIAL_MAP_EMISSION].color = rawMaterial.emissiveColor;
                
                if (!rawMaterial.emissivePath.empty() && project) {
                    auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(rawMaterial.emissivePath);
                    if (textureAsset && textureAsset->IsReady()) {
                        model.materials[i].maps[MATERIAL_MAP_EMISSION].texture = textureAsset->GetTexture();
                        m_Textures.push_back(textureAsset);
                    }
                    else if (textureAsset) {
                        m_PendingTextures.push_back({i, rawMaterial.emissivePath, MATERIAL_MAP_EMISSION});
                    }
                }

                // Handle Normal
                if (!rawMaterial.normalPath.empty() && project) {
                    auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(rawMaterial.normalPath);
                    if (textureAsset && textureAsset->IsReady()) {
                        model.materials[i].maps[MATERIAL_MAP_NORMAL].texture = textureAsset->GetTexture();
                        m_Textures.push_back(textureAsset);
                    }
                    else if (textureAsset) {
                        m_PendingTextures.push_back({i, rawMaterial.normalPath, MATERIAL_MAP_NORMAL});
                    }
                }

                // Handle Metallic/Roughness (Packed)
                model.materials[i].maps[MATERIAL_MAP_METALNESS].value = rawMaterial.metalness;
                model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].value = rawMaterial.roughness;

                if (!rawMaterial.metallicRoughnessPath.empty() && project) {
                    auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(rawMaterial.metallicRoughnessPath);
                    if (textureAsset && textureAsset->IsReady()) {
                        model.materials[i].maps[MATERIAL_MAP_METALNESS].texture = textureAsset->GetTexture();
                        model.materials[i].maps[MATERIAL_MAP_ROUGHNESS].texture = textureAsset->GetTexture();
                        m_Textures.push_back(textureAsset);
                    }
                    else if (textureAsset) {
                        m_PendingTextures.push_back({i, rawMaterial.metallicRoughnessPath, MATERIAL_MAP_METALNESS});
                        m_PendingTextures.push_back({i, rawMaterial.metallicRoughnessPath, MATERIAL_MAP_ROUGHNESS});
                    }
                }

                // Handle Occlusion
                if (!rawMaterial.occlusionPath.empty() && project) {
                    auto textureAsset = project->GetAssetManager()->Get<TextureAsset>(rawMaterial.occlusionPath);
                    if (textureAsset && textureAsset->IsReady()) {
                        model.materials[i].maps[MATERIAL_MAP_OCCLUSION].texture = textureAsset->GetTexture();
                        m_Textures.push_back(textureAsset);
                    }
                    else if (textureAsset) {
                        m_PendingTextures.push_back({i, rawMaterial.occlusionPath, MATERIAL_MAP_OCCLUSION});
                    }
                }
            }
        }

        // Load Meshes
        for (int i = 0; i < model.meshCount; ++i) {
            const auto& rawMesh = m_PendingData.meshes[i];
            Mesh mesh = {0};
            mesh.vertexCount = (int)rawMesh.vertices.size() / 3;
            mesh.triangleCount = (int)rawMesh.indices.size() / 3;
            
            if (mesh.vertexCount > 0) {
                mesh.vertices = (float*)RL_MALLOC(rawMesh.vertices.size() * sizeof(float));
                std::memcpy(mesh.vertices, rawMesh.vertices.data(), rawMesh.vertices.size() * sizeof(float));
                
                if (!rawMesh.texcoords.empty()) {
                    mesh.texcoords = (float*)RL_MALLOC(rawMesh.texcoords.size() * sizeof(float));
                    std::memcpy(mesh.texcoords, rawMesh.texcoords.data(), rawMesh.texcoords.size() * sizeof(float));
                }

                if (!rawMesh.normals.empty()) {
                    mesh.normals = (float*)RL_MALLOC(rawMesh.normals.size() * sizeof(float));
                    std::memcpy(mesh.normals, rawMesh.normals.data(), rawMesh.normals.size() * sizeof(float));
                }

                if (!rawMesh.indices.empty()) {
                    mesh.indices = (unsigned short*)RL_MALLOC(rawMesh.indices.size() *sizeof(unsigned short));
                    std::memcpy(mesh.indices, rawMesh.indices.data(), rawMesh.indices.size() * sizeof(unsigned short));
                }

                if (!rawMesh.colors.empty()) {
                    mesh.colors = (unsigned char*)RL_MALLOC(rawMesh.colors.size() * sizeof(unsigned char));
                    std::memcpy(mesh.colors, rawMesh.colors.data(), rawMesh.colors.size() * sizeof(unsigned char));
                }

                if (!rawMesh.tangents.empty()) {
                    mesh.tangents = (float*)RL_MALLOC(rawMesh.tangents.size() * sizeof(float));
                    std::memcpy(mesh.tangents, rawMesh.tangents.data(), rawMesh.tangents.size() * sizeof(float));
                }

                if (!rawMesh.joints.empty()) {
                    mesh.boneIds = (unsigned char*)RL_MALLOC(rawMesh.joints.size() * sizeof(unsigned char));
                    std::memcpy(mesh.boneIds, rawMesh.joints.data(), rawMesh.joints.size() * sizeof(unsigned char));
                }

                if (!rawMesh.weights.empty()) {
                    mesh.boneWeights = (float*)RL_MALLOC(rawMesh.weights.size() * sizeof(float));
                    std::memcpy(mesh.boneWeights, rawMesh.weights.data(), rawMesh.weights.size() * sizeof(float));
                }

                UploadMesh(&mesh, false);

                // If no tangents but we have normals and texcoords, generate them
                if (mesh.tangents == nullptr && mesh.normals != nullptr && mesh.texcoords != nullptr) {
                    GenMeshTangents(&mesh);
                    // Re-upload with tangents
                    UpdateMeshBuffer(mesh, 4, mesh.tangents, mesh.vertexCount * 4 * sizeof(float), 0);
                }
            }
            
            model.meshes[i] = mesh;
            model.meshMaterial[i] = (rawMesh.materialIndex >= 0 && rawMesh.materialIndex < model.materialCount) ? rawMesh.materialIndex : 0;
        }

        model.transform = MatrixIdentity();

        // Lock and transfer
        {
            std::lock_guard<std::mutex> lock(m_ModelMutex);
            m_Model = model;
            
            // Transfer animations
            m_Animations = m_PendingData.animations;
            m_AnimationCount = m_PendingData.animationCount;
            
            // If we have animations, the model needs bones for UpdateModelAnimation to work
            if (m_AnimationCount > 0 && m_Animations[0].boneCount > 0)
            {
                m_Model.boneCount = m_Animations[0].boneCount;
                m_Model.bones = (BoneInfo*)RL_MALLOC(m_Model.boneCount * sizeof(BoneInfo));
                std::memcpy(m_Model.bones, m_Animations[0].bones, m_Model.boneCount * sizeof(BoneInfo));
                
                // Use first frame of first animation as bind pose if no better option
                m_Model.bindPose = (Transform*)RL_MALLOC(m_Model.boneCount * sizeof(Transform));
                std::memcpy(m_Model.bindPose, m_Animations[0].framePoses[0], m_Model.boneCount * sizeof(Transform));
            }
        }
        
        m_PendingData = PendingModelData();
        m_HasPendingData = false;
        
        SetState(AssetState::Ready);
        CH_CORE_INFO("ModelAsset: GPU upload completed for '{}'", GetPath());
    }

    ModelAsset::~ModelAsset()
    {
        if (m_Model.meshCount > 0) ::UnloadModel(m_Model);
        if (m_Animations) ::UnloadModelAnimations(m_Animations, m_AnimationCount);
    }

    void ModelAsset::UpdateAnimation(int animationIndex, int frame)
    {
        if (m_Animations && animationIndex < m_AnimationCount)
        {
            ::UpdateModelAnimation(m_Model, m_Animations[animationIndex], frame);
        }
    }

    void ModelAsset::OnUpdate()
    {
        if (m_PendingTextures.empty()) return;

        auto project = Project::GetActive();
        if (!project) return;

        auto assetManager = project->GetAssetManager();

        std::lock_guard<std::mutex> lock(m_ModelMutex);

        for (auto it = m_PendingTextures.begin(); it != m_PendingTextures.end(); )
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
    ModelAnimation *ModelAsset::GetAnimations(int *count)
    {
        if (count) *count = m_AnimationCount;
        return m_Animations;
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

    int ModelAsset::GetAnimationCount() const
    {
        return m_AnimationCount;
    }

    const std::vector<std::shared_ptr<TextureAsset>> &ModelAsset::GetTextures() const
    {
        return m_Textures;
    }

} // namespace CHEngine

