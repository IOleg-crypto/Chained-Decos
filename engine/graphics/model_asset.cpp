#include "model_asset.h"
#include "engine/core/log.h"
#include "engine/physics/bvh/bvh.h"
#include "raylib.h"
#include "shader_asset.h"
#include "texture_asset.h"
#include "engine/graphics/api_context.h"
#include <filesystem>
#include <functional>
#include <unordered_map>

namespace CHEngine
{
std::shared_ptr<ModelAsset> ModelAsset::Load(const std::string &path)
{
    auto asset = std::make_shared<ModelAsset>();
    asset->SetPath(path);
    asset->LoadFromFile(path);
    if (asset->GetState() == AssetState::Ready) return asset;
    return nullptr;
}

void ModelAsset::LoadFromFile(const std::string &path)
{
    if (path.empty()) 
    {
        SetState(AssetState::Failed);
        return;
    }

    if (path.starts_with(":"))
    {
        this->m_Model = GenerateProceduralModel(path);
        if (this->m_Model.meshCount > 0)
        {
            SetState(AssetState::Ready);
        }
        else SetState(AssetState::Failed);
        return;
    }

    std::filesystem::path fullPath(path);
    if (!std::filesystem::exists(fullPath))
    {
        CH_CORE_ERROR("ModelAsset: Path does not exist: '{}'", path);
        SetState(AssetState::Failed);
        return;
    }

    std::string pathStr = std::filesystem::absolute(fullPath).string();
    Model model = ::LoadModel(pathStr.c_str());
    
    if (model.meshCount == 0)
    {
        CH_CORE_ERROR("ModelAsset: ::LoadModel failed for '{}'", path);
        SetState(AssetState::Failed);
        return;
    }

    this->m_Model = model;
    this->SetPath(path);

    // Load animations
    int animsCount = 0;
    this->m_Animations = LoadModelAnimations(pathStr.c_str(), &animsCount);
    this->m_AnimCount = animsCount;

    // Ensure materials have a shader
    auto &state = APIContext::GetState();
    if (this->m_Model.materials)
    {
        for (int i = 0; i < this->m_Model.materialCount; i++)
        {
            if (this->m_Model.materials[i].shader.id == 0 && state.LightingShader)
                this->m_Model.materials[i].shader = state.LightingShader->GetShader();
        }
    }

    SetState(AssetState::Ready);
}

void ModelAsset::UploadToGPU()
{
    // All work is done in Load/LoadFromFile in this simplified version
}

Model ModelAsset::GenerateProceduralModel(const std::string &type)
{
    static const std::unordered_map<std::string, std::function<Mesh()>> s_Generators = {
        {":cube:",       []() { return GenMeshCube(1.0f, 1.0f, 1.0f); }},
        {":sphere:",     []() { return GenMeshSphere(0.5f, 16, 16); }},
        {":plane:",      []() { return GenMeshPlane(10.0f, 10.0f, 10, 10); }},
        {":torus:",      []() { return GenMeshTorus(0.2f, 0.4f, 16, 16); }},
        {":cylinder:",   []() { return GenMeshCylinder(0.5f, 1.0f, 16); }},
        {":cone:",       []() { return GenMeshCone(0.5f, 1.0f, 16); }},
        {":knot:",       []() { return GenMeshKnot(0.5f, 0.2f, 16, 128); }},
        {":hemisphere:", []() { return GenMeshHemiSphere(0.5f, 16, 16); }}
    };

    auto it = s_Generators.find(type);
    if (it == s_Generators.end()) return {0};

    Mesh mesh = it->second();
    if (mesh.vertexCount == 0) return {0};

    return LoadModelFromMesh(mesh);
}

std::shared_ptr<ModelAsset> ModelAsset::CreateProcedural(const std::string &type)
{
    Model model = GenerateProceduralModel(type);
    if (model.meshCount == 0) return nullptr;

    auto asset = std::make_shared<ModelAsset>();
    asset->m_Model = model;
    asset->SetPath(type);
    asset->SetState(AssetState::Ready);
    return asset;
}

ModelAsset::~ModelAsset()
{
    if (m_Model.meshCount > 0) ::UnloadModel(m_Model);
    if (m_Animations) ::UnloadModelAnimations(m_Animations, m_AnimCount);
}

void ModelAsset::UpdateAnimation(int animIndex, int frame)
{
    if (m_Animations && animIndex < m_AnimCount && m_Model.boneCount > 0)
        UpdateModelAnimation(m_Model, m_Animations[animIndex], frame);
}

ModelAnimation *ModelAsset::GetAnimations(int *count)
{
    if (count) *count = m_AnimCount;
    return m_Animations;
}

BoundingBox ModelAsset::GetBoundingBox() const
{
    return ::GetModelBoundingBox(m_Model);
}


AssetType ModelAsset::GetType() const
{
    return AssetType::Model;
}

Model &ModelAsset::GetModel()
{
    return m_Model;
}

const Model &ModelAsset::GetModel() const
{
    return m_Model;
}

int ModelAsset::GetAnimationCount() const
{
    return m_AnimCount;
}

const std::vector<std::shared_ptr<TextureAsset>> &ModelAsset::GetTextures() const
{
    return m_Textures;
}

} // namespace CHEngine
