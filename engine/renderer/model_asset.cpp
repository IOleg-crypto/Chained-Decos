#include "model_asset.h"
#include "asset_manager.h"
#include "engine/core/log.h"
#include "texture_asset.h"
#include <filesystem>
#include <raylib.h>

namespace CHEngine
{
Ref<ModelAsset> ModelAsset::Load(const std::string &path)
{
    if (path.empty())
        return nullptr;

    // Check for procedural models first
    if (path.starts_with(":"))
    {
        return CreateProcedural(path);
    }

    auto fullPath = Assets::ResolvePath(path);
    CH_CORE_INFO("Loading model: {} (resolved: {})", path, fullPath.string());

    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("Model file not found: {}", fullPath.string());
        return nullptr;
    }

    Model model = ::LoadModel(fullPath.string().c_str());
    if (model.meshCount == 0)
    {
        CH_CORE_ERROR("Failed to load model meshes: {}", path);
        return nullptr;
    }

    CH_CORE_INFO("Model loaded successfully: {} (meshes: {}, materials: {})", path, model.meshCount,
                 model.materialCount);

    auto asset = CreateRef<ModelAsset>();
    asset->m_Model = model;
    asset->SetPath(path);

    // Register textures with AssetManager to ensure they are tracked
    for (int i = 0; i < model.materialCount; i++)
    {
        for (int j = 0; j < 12; j++) // MAX_MATERIAL_MAPS is 12 in Raylib
        {
            Texture2D tex = model.materials[i].maps[j].texture;
            if (tex.id > 0)
            {
                // We create a wrapper but we don't know the original path here unfortunately
                // However, we can track it as a managed texture
                auto texAsset = CreateRef<TextureAsset>();
                texAsset->SetTexture(tex);
                asset->m_Textures.push_back(texAsset);
            }
        }
    }

    // Load animations - Raylib animations are sometimes problematic with GLB
    CH_CORE_INFO("Loading animations for: {}", path);
    asset->m_Animations = ::LoadModelAnimations(fullPath.string().c_str(), &asset->m_AnimCount);
    CH_CORE_INFO("Animations loaded for: {} (count: {})", path, asset->m_AnimCount);

    return asset;
}

Ref<ModelAsset> ModelAsset::CreateProcedural(const std::string &type)
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

    auto asset = CreateRef<ModelAsset>();
    asset->m_Model = LoadModelFromMesh(mesh);
    asset->SetPath(type);
    return asset;
}

ModelAsset::~ModelAsset()
{
    if (m_Model.meshCount > 0)
        ::UnloadModel(m_Model);
    if (m_Animations)
        ::UnloadModelAnimations(m_Animations, m_AnimCount);

    CH_CORE_TRACE("ModelAsset Unloaded: {}", GetPath());
}

void ModelAsset::UpdateAnimation(int animIndex, int frame)
{
    if (m_Animations && animIndex < m_AnimCount)
    {
        UpdateModelAnimation(m_Model, m_Animations[animIndex], frame);
    }
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
