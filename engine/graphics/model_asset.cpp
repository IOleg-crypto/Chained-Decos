#include "model_asset.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "raylib.h"
#include "texture_asset.h"
#include <filesystem>

namespace CHEngine
{
std::shared_ptr<ModelAsset> ModelAsset::Load(const std::string &path)
{
    if (path.empty())
        return nullptr;
    if (path.starts_with(":"))
        return CreateProcedural(path);

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
        return nullptr;

    Model model = ::LoadModel(fullPath.string().c_str());
    if (model.meshCount == 0)
        return nullptr;

    auto asset = std::make_shared<ModelAsset>();
    asset->m_Model = model;
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);

    asset->m_BVHFuture = BVH::BuildAsync(model).share();

    // Track internal textures
    for (int i = 0; i < model.materialCount; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            Texture2D tex = model.materials[i].maps[j].texture;
            if (tex.id > 0)
            {
                auto texAsset = std::make_shared<TextureAsset>();
                texAsset->SetTexture(tex);
                texAsset->SetState(AssetState::Ready);
                asset->m_Textures.push_back(texAsset);
            }
        }
    }
    return asset;
}

void ModelAsset::LoadAsync(const std::string &path)
{
    // For now, simpler models just load synchronously in background thread
    // then upload to GPU using a simplified approach
    // Raylib's LoadModel is mostly CPU until the final VBO upload

    // AssetManager is gone, so for now we just load and throw away or implement new system
    // But since this is a static method, we don't have a place to stash it yet.
    // For baseline build, we just load.
    auto loaded = Load(path);
}

void ModelAsset::UploadToGPU()
{
}

std::shared_ptr<ModelAsset> ModelAsset::CreateProcedural(const std::string &type)
{
    Mesh mesh = {0};
    if (type == ":cube:")
        mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    else if (type == ":sphere:")
        mesh = GenMeshSphere(0.5f, 16, 16);
    else if (type == ":plane:")
        mesh = GenMeshPlane(10.0f, 10.0f, 10, 10);
    else if (type == ":torus:")
        mesh = GenMeshTorus(0.2f, 0.4f, 16, 16);

    if (mesh.vertexCount == 0)
        return nullptr;

    auto asset = std::make_shared<ModelAsset>();
    asset->m_Model = LoadModelFromMesh(mesh);
    asset->SetPath(type);
    asset->SetState(AssetState::Ready);
    asset->m_BVHFuture = BVH::BuildAsync(asset->m_Model).share();
    return asset;
}

ModelAsset::~ModelAsset()
{
    if (m_Model.meshCount > 0)
        ::UnloadModel(m_Model);
    if (m_Animations)
        ::UnloadModelAnimations(m_Animations, m_AnimCount);
}

void ModelAsset::UpdateAnimation(int animIndex, int frame)
{
    if (m_Animations && animIndex < m_AnimCount)
        UpdateModelAnimation(m_Model, m_Animations[animIndex], frame);
}

ModelAnimation *ModelAsset::GetAnimations(int *count)
{
    if (count)
        *count = m_AnimCount;
    return m_Animations;
}

BoundingBox ModelAsset::GetBoundingBox() const
{
    return ::GetModelBoundingBox(m_Model);
}
} // namespace CHEngine
