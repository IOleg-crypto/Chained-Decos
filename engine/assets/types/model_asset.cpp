#include "engine/assets/types/model_asset.h"
#include "engine/assets/types/texture_asset.h"
#include "engine/graphics/api/buffer.h"
#include "engine/graphics/api/vertex_array.h"
#include "engine/graphics/api/storage_buffer.h"
#include "engine/core/log.h"
#include "engine/project/project.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Chained
{
std::string ModelAsset::GetAnimationName(int index) const
{
    if (index >= 0 && index < (int)m_Animations.size())
    {
        return std::string(m_Animations[index].name.c_str());
    }
    return "";
}

std::vector<glm::mat4> ModelAsset::GetBoneMatrices(int animationIndex, int frame) const
{
    if (animationIndex < 0 || animationIndex >= m_Animations.size())
    {
        return {};
    }
    const auto& anim = m_Animations[animationIndex];
    if (frame < 0 || frame >= anim.frameCount)
    {
        return {};
    }

    int boneCount = anim.boneCount;
    if (boneCount == 0)
    {
        return {};
    }

    std::vector<glm::mat4> globalTransforms(boneCount);
    std::vector<glm::mat4> finalMatrices;
    finalMatrices.reserve(boneCount);

    for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
    {
        const auto& pose = anim.framePoses[frame * boneCount + boneIndex];

        glm::mat4 local = glm::translate(glm::mat4(1.0f), pose.translation) * glm::mat4_cast(pose.rotation) *
                          glm::scale(glm::mat4(1.0f), pose.scale);

        if (m_NodeParents[boneIndex] == -1)
        {
            globalTransforms[boneIndex] = local;
        }
        else
        {
            globalTransforms[boneIndex] = globalTransforms[m_NodeParents[boneIndex]] * local;
        }

        glm::mat4 offset = (boneIndex < (int)m_OffsetMatrices.size()) ? m_OffsetMatrices[boneIndex] : glm::mat4(1.0f);
        finalMatrices.push_back(globalTransforms[boneIndex] * offset);
    }

    return finalMatrices;
}

void ModelAsset::OnLoaded()
{
    CH_CORE_INFO("Loaded Model asset");
}

uint32_t ModelAsset::GetEmbeddedTextureID(const std::string& path) const
{
    auto it = m_EmbeddedTextures.find(path);
    if (it != m_EmbeddedTextures.end() && it->second)
    {
        return it->second->GetRendererID();
    }
    return 0;
}

bool ModelAsset::Finalize()
{
    if (!m_HasPendingData)
        return m_Model.Meshes.size() > 0;

    m_Model.Meshes.clear();
    m_Model.Materials.clear();
    
    for (const auto& rawMat : m_PendingData.materials)
    {
        Material mat;
        mat.AlbedoColor = rawMat.albedoColor;
        // The texture paths should be resolved and stored as handles by a material system later.
        mat.Roughness = rawMat.roughness;
        mat.Metalness = rawMat.metalness;
        m_Model.Materials.push_back(mat);
    }

    for (const auto& rawMesh : m_PendingData.meshes)
    {
        CH_CORE_INFO("Finalize: Processing Mesh");
        Mesh mesh;
        mesh.MaterialIndex = rawMesh.materialIndex;
        mesh.VertexCount = (uint32_t)(rawMesh.vertices.size() / 3);
        mesh.TriangleCount = (uint32_t)(rawMesh.indices.size() / 3);
        mesh.MinBounds = rawMesh.MinBounds;
        mesh.MaxBounds = rawMesh.MaxBounds;

        CH_CORE_INFO("Finalize: Creating VAO");
        mesh.VAO = VertexArray::Create();

        CH_CORE_INFO("Finalize: Creating posVbo, count={}", rawMesh.vertices.size());
        if (!rawMesh.vertices.empty()) 
        {
            auto posVbo = VertexBuffer::Create(rawMesh.vertices.data(), (uint32_t)(rawMesh.vertices.size() * sizeof(float)));
            posVbo->SetLayout({{ShaderDataType::Float3, "a_Position"}});
            mesh.VAO->AddVertexBuffer(posVbo);
        }

        CH_CORE_INFO("Finalize: Creating texVbo");
        if (!rawMesh.texcoords.empty())
        {
            auto texVbo = VertexBuffer::Create(rawMesh.texcoords.data(), (uint32_t)(rawMesh.texcoords.size() * sizeof(float)));
            texVbo->SetLayout({{ShaderDataType::Float2, "a_TexCoord"}});
            mesh.VAO->AddVertexBuffer(texVbo);
        }

        if (!rawMesh.normals.empty())
        {
            auto normVbo = VertexBuffer::Create(rawMesh.normals.data(), (uint32_t)(rawMesh.normals.size() * sizeof(float)));
            normVbo->SetLayout({{ShaderDataType::Float3, "a_Normal"}});
            mesh.VAO->AddVertexBuffer(normVbo);
        }

        if (!rawMesh.tangents.empty())
        {
            auto tanVbo = VertexBuffer::Create(rawMesh.tangents.data(), (uint32_t)(rawMesh.tangents.size() * sizeof(float)));
            tanVbo->SetLayout({{ShaderDataType::Float3, "a_Tangent"}});
            mesh.VAO->AddVertexBuffer(tanVbo);
        }

        if (!rawMesh.joints.empty())
        {
            // joints is unsigned char. Cast to int or float buffer? 
            // We should convert unsigned char array to float array for VBO upload, as VertexBuffer::Create expects float
            std::vector<float> floatJoints(rawMesh.joints.begin(), rawMesh.joints.end());
            auto jointsVbo = VertexBuffer::Create((const float*)floatJoints.data(), (uint32_t)(floatJoints.size() * sizeof(float)));
            jointsVbo->SetLayout({{ShaderDataType::Float4, "a_Joints"}});
            mesh.VAO->AddVertexBuffer(jointsVbo);
        }

        if (!rawMesh.weights.empty())
        {
            auto weightsVbo = VertexBuffer::Create((const float*)rawMesh.weights.data(), (uint32_t)(rawMesh.weights.size() * sizeof(float)));
            weightsVbo->SetLayout({{ShaderDataType::Float4, "a_Weights"}});
            mesh.VAO->AddVertexBuffer(weightsVbo);
        }

        CH_CORE_INFO("Finalize: Creating ebo");
        if (!rawMesh.indices.empty()) 
        {
            auto ebo = IndexBuffer::Create((uint32_t*)rawMesh.indices.data(), (uint32_t)rawMesh.indices.size());
            mesh.VAO->SetIndexBuffer(ebo);
        }

        m_Model.Meshes.push_back(mesh);
    }

    CH_CORE_INFO("Finalize: Finshing metadata");
    m_Animations = m_PendingData.animations;
    m_Instances = m_PendingData.instances;
    m_NodeNames = m_PendingData.nodeNames;
    m_NodeParents = m_PendingData.nodeParents;
    m_OffsetMatrices = m_PendingData.offsetMatrices;

    m_RawMeshes = std::move(m_PendingData.meshes);
    m_PendingData = PendingModelData(); // clear memory
    m_HasPendingData = false;

    SetState(AssetState::Ready);
    CH_CORE_INFO("ModelAsset::Finalize - Generated OpenGL buffers for ModelAsset");
    return true;
}

} // namespace Chained
