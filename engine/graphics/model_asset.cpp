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

void ModelAsset::LoadFromFile(const std::string &path)
{
    if (m_State == AssetState::Ready) return;

    if (path.starts_with(":"))
    {
        // Procedural models: Create mesh on CPU (safe in background thread)
        // GenMesh* functions only allocate CPU memory, no GPU upload
        Mesh mesh = {0};
        if (path == ":cube:")
            mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
        else if (path == ":sphere:")
            mesh = GenMeshSphere(0.5f, 16, 16);
        else if (path == ":plane:")
            mesh = GenMeshPlane(10.0f, 10.0f, 10, 10);
        else if (path == ":torus:")
            mesh = GenMeshTorus(0.2f, 0.4f, 16, 16);
        
        if (mesh.vertexCount > 0)
        {
            m_PendingMesh = mesh;
            m_HasPendingMesh = true;
            // Stay in Loading state until UploadToGPU completes
        }
        else
        {
            SetState(AssetState::Failed);
        }
        return;
    }

    // File-based models: Raylib's LoadModel does GPU upload, so we can't call it here
    // Instead, just validate file exists and defer loading to UploadToGPU
    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        SetState(AssetState::Failed);
        return;
    }

    m_PendingModelPath = path;
    m_HasPendingModel = true;
    // Stay in Loading state until UploadToGPU completes on main thread
}

void ModelAsset::UploadToGPU()
{
    // This MUST run on main thread where OpenGL context exists
    
    if (m_HasPendingMesh)
    {
        // Upload procedural mesh to GPU
        m_Model = LoadModelFromMesh(m_PendingMesh);
        m_HasPendingMesh = false;
        m_PendingMesh = {0};
        
        if (m_Model.meshCount > 0)
        {
            // Start BVH build async (CPU work, safe)
            m_BVHFuture = BVH::BuildAsync(m_Model).share();
            SetState(AssetState::Ready);  // ✅ NOW it's ready for rendering
        }
        else
        {
            SetState(AssetState::Failed);
        }
    }
    
    if (m_HasPendingModel)
    {
        // Load file-based model (includes GPU upload by Raylib)
        m_Model = ::LoadModel(m_PendingModelPath.c_str());
        m_HasPendingModel = false;
        m_PendingModelPath.clear();
        
        if (m_Model.meshCount > 0)
        {
            // Start BVH build async
            m_BVHFuture = BVH::BuildAsync(m_Model).share();
            
            // Track internal textures
            for (int i = 0; i < m_Model.materialCount; i++)
            {
                for (int j = 0; j < 12; j++)
                {
                    Texture2D tex = m_Model.materials[i].maps[j].texture;
                    if (tex.id > 0)
                    {
                        auto texAsset = std::make_shared<TextureAsset>();
                        texAsset->SetTexture(tex);
                        texAsset->SetState(AssetState::Ready);
                        m_Textures.push_back(texAsset);
                    }
                }
            }
            
            SetState(AssetState::Ready);  
        }
        else
        {
            SetState(AssetState::Failed);
        }
    }
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
