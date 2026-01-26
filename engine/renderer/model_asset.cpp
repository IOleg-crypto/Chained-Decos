#include "model_asset.h"
#include "asset_manager.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "texture_asset.h"
#include <filesystem>
#include <raylib.h>

namespace CHEngine
{
std::shared_ptr<ModelAsset> ModelAsset::Load(const std::string &path)
{
    if (path.empty())
        return nullptr;
    if (path.starts_with(":"))
        return CreateProcedural(path);

    auto fullPath = AssetManager::ResolvePath(path);
    if (!std::filesystem::exists(fullPath))
        return nullptr;

    Model model = ::LoadModel(fullPath.string().c_str());
    if (model.meshCount == 0)
        return nullptr;

    auto asset = std::make_shared<ModelAsset>();
    asset->m_Model = model;
    asset->SetPath(path);
    asset->SetState(AssetState::Ready);

    asset->m_BVHFuture = BVHBuilder::BuildAsync(model);

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
    auto asset = AssetManager::Get<ModelAsset>(path);
    if (!asset)
        return;

    auto fullPath = AssetManager::ResolvePath(path);
    if (!std::filesystem::exists(fullPath))
    {
        asset->SetState(AssetState::Failed);
        return;
    }

    // NOTE: LoadModel will perform GPU allocation if called on background thread in standard
    // setups, but in our ChainedThread system we need to be careful. Ideally we'd use LoadMesh
    // directly then upload. For simplicity, we'll keep it as a managed sync for now, but the
    // AssetManager::Update will call UploadToGPU if we split it.

    // For this refactor, we'll focus on the TextureAsset first as proof of concept.
    // ModelAsset Load will stay as-is but marked Ready.
    auto loaded = Load(path);
    if (loaded)
    {
        asset->m_Model = loaded->m_Model;
        asset->m_Animations = loaded->m_Animations;
        asset->m_AnimCount = loaded->m_AnimCount;
        asset->m_Textures = loaded->m_Textures;
        asset->SetState(AssetState::Ready);
    }
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
    asset->m_BVHFuture = BVHBuilder::BuildAsync(asset->m_Model);
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
