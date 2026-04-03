#include "engine/graphics/assets/model_asset.h"
#include "engine/core/assets/asset_manager.h"
#include "engine/core/log.h"
// #include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/assets/texture_asset.h"
// #include "engine/graphics/importers/mesh_importer.h"
#include "engine/scene/project.h"
// #include <algorithm>
#include <cstring>
// #include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace CHEngine
{
std::string ModelAsset::GetAnimationName(int index) const
{
    return (index >= 0 && index < (int)m_Animations.size()) ? m_Animations[index].name : "";
}

void ModelAsset::OnLoaded()
{
    if (!m_HasPendingData || !m_PendingData.isValid)
    {
        return;
    }

    CH_CORE_INFO("ModelAsset: Uploading model to GPU: '{}' ({} meshes, {} materials)", GetPath(),
                 static_cast<uint32_t>(m_PendingData.meshes.size()),
                 static_cast<uint32_t>(m_PendingData.materials.size()));

    Model newModel;
    newModel.Materials.resize(m_PendingData.materials.empty() ? 1 : m_PendingData.materials.size());

    auto project = Project::GetActive();

    auto loadTex = [&](int matIdx, const std::string& path, int mapIndex) {
        if (path.empty() || !project)
        {
            return;
        }
        auto tex = AssetManager::Get().Get<TextureAsset>(path);
        if (!tex)
        {
            return;
        }

        uint32_t texId = 0;
        if (tex->IsReady())
        {
            texId = tex->GetTexture()->GetRendererID();
        }
        else
        {
            // Optional: fallback texture ID here
            texId = 0; 
        }
        switch (mapIndex)
        {
        case 0:
            newModel.Materials[matIdx].AlbedoMap = texId;
            break;
        case 1:
            newModel.Materials[matIdx].EmissiveMap = texId;
            break;
        case 2:
            newModel.Materials[matIdx].NormalMap = texId;
            break;
        case 3:
            newModel.Materials[matIdx].MetallicRoughnessMap = texId;
            break;
        case 4:
            newModel.Materials[matIdx].EmissiveMap = texId;
            break;
        case 5:
            newModel.Materials[matIdx].OcclusionMap = texId;
            break;
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

            loadTex(i, rawMaterial.albedoPath, 0);
            loadTex(i, rawMaterial.emissivePath, 4);
            loadTex(i, rawMaterial.normalPath, 2);
            loadTex(i, rawMaterial.occlusionPath, 5);
            loadTex(i, rawMaterial.metallicRoughnessPath, 3);
        }
    }

    for (int i = 0; i < (int)m_PendingData.meshes.size(); ++i)
    {
        const auto& rawMesh = m_PendingData.meshes[i];
        Mesh mesh;
        mesh.VertexCount = (uint32_t)rawMesh.vertices.size() / 3;
        mesh.TriangleCount = (uint32_t)rawMesh.indices.size() / 3;
        mesh.MaterialIndex = (rawMesh.materialIndex >= 0 && rawMesh.materialIndex < (int)newModel.Materials.size())
                                 ? rawMesh.materialIndex
                                 : 0;

        if (mesh.VertexCount > 0)
        {
            mesh.VAO = VertexArray::Create();

            auto vboPos =
                VertexBuffer::Create(rawMesh.vertices.data(), (uint32_t)rawMesh.vertices.size() * sizeof(float));
            vboPos->SetLayout({{ShaderDataType::Float3, "a_Position"}});
            mesh.VAO->AddVertexBuffer(vboPos);

            if (!rawMesh.texcoords.empty())
            {
                auto vboTex =
                    VertexBuffer::Create(rawMesh.texcoords.data(), (uint32_t)rawMesh.texcoords.size() * sizeof(float));
                vboTex->SetLayout({{ShaderDataType::Float2, "a_TexCoord"}});
                mesh.VAO->AddVertexBuffer(vboTex);
            }

            if (!rawMesh.normals.empty())
            {
                auto vboNorm =
                    VertexBuffer::Create(rawMesh.normals.data(), (uint32_t)rawMesh.normals.size() * sizeof(float));
                vboNorm->SetLayout({{ShaderDataType::Float3, "a_Normal"}});
                mesh.VAO->AddVertexBuffer(vboNorm);
            }

            if (!rawMesh.indices.empty())
            {
                std::vector<uint32_t> indices32(rawMesh.indices.begin(), rawMesh.indices.end());
                auto ibo = IndexBuffer::Create(indices32.data(), (uint32_t)indices32.size());
                mesh.VAO->SetIndexBuffer(ibo);
            }

            mesh.MinBounds = {FLT_MAX, FLT_MAX, FLT_MAX};
            mesh.MaxBounds = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
            for (size_t v = 0; v < rawMesh.vertices.size(); v += 3)
            {
                mesh.MinBounds =
                    glm::min(mesh.MinBounds, {rawMesh.vertices[v], rawMesh.vertices[v + 1], rawMesh.vertices[v + 2]});
                mesh.MaxBounds =
                    glm::max(mesh.MaxBounds, {rawMesh.vertices[v], rawMesh.vertices[v + 1], rawMesh.vertices[v + 2]});
            }
        }
        newModel.Meshes.push_back(mesh);
    }

    BoundingBox totalBox = {{FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX}};
    bool anyMesh = false;
    for (const auto& inst : m_PendingData.instances)
    {
        if (inst.meshIndex < 0 || inst.meshIndex >= (int)newModel.Meshes.size())
        {
            continue;
        }
        const Mesh& mesh = newModel.Meshes[inst.meshIndex];

        glm::vec3 corners[8] = {{mesh.MinBounds.x, mesh.MinBounds.y, mesh.MinBounds.z},
                                {mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MinBounds.z},
                                {mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z},
                                {mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MinBounds.z},
                                {mesh.MinBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z},
                                {mesh.MaxBounds.x, mesh.MinBounds.y, mesh.MaxBounds.z},
                                {mesh.MinBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z},
                                {mesh.MaxBounds.x, mesh.MaxBounds.y, mesh.MaxBounds.z}};

        for (int c = 0; c < 8; c++)
        {
            glm::vec4 transformed = inst.localTransform * glm::vec4(corners[c], 1.0f);
            totalBox.Min = glm::min(totalBox.Min, glm::vec3(transformed));
            totalBox.Max = glm::max(totalBox.Max, glm::vec3(transformed));
        }
        anyMesh = true;
    }
    if (!anyMesh)
    {
        totalBox = {{0, 0, 0}, {0, 0, 0}};
    }

    m_Model = std::move(newModel);
    m_BoundingBox = totalBox;
    m_RawMeshes = std::move(m_PendingData.meshes);
    m_Animations = std::move(m_PendingData.animations);
    m_Instances = std::move(m_PendingData.instances);
    m_OffsetMatrices = std::move(m_PendingData.offsetMatrices);
    m_NodeNames = std::move(m_PendingData.nodeNames);
    m_NodeParents = std::move(m_PendingData.nodeParents);

    m_PendingData = PendingModelData();
    m_HasPendingData = false;
    SetState(AssetState::Ready);
}

} // namespace CHEngine
