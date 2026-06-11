#include "engine/graphics/assets/model_asset.h"
#include "engine/graphics/loaders/model_loader.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
// #include "engine/graphics/assets/shader_asset.h"
#include "engine/graphics/assets/texture_asset.h"
// #include "engine/graphics/importers/mesh_importer.h"
#include "engine/project/project.h"
// #include <algorithm>
#include <cstring>
// #include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace CHEngine
{
std::string ModelAsset::GetAnimationName(int index) const
{
    if (index >= 0 && index < (int)m_Animations.size())
        return std::string(m_Animations[index].name.c_str());
    return "";
}

std::vector<glm::mat4> ModelAsset::GetBoneMatrices(int animationIndex, int frame) const
{
    if (animationIndex < 0 || animationIndex >= m_Animations.size()) return {};
    const auto& anim = m_Animations[animationIndex];
    if (frame < 0 || frame >= anim.frameCount) return {};

    int boneCount = anim.boneCount;
    if (boneCount == 0) return {};

    std::vector<glm::mat4> globalTransforms(boneCount);
    std::vector<glm::mat4> finalMatrices;
    finalMatrices.reserve(boneCount);

    for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
    {
        const auto& pose = anim.framePoses[frame * boneCount + boneIndex];
        
        glm::mat4 local = glm::translate(glm::mat4(1.0f), pose.translation) *
                          glm::mat4_cast(pose.rotation) *
                          glm::scale(glm::mat4(1.0f), pose.scale);

        if (m_NodeParents[boneIndex] == -1)
            globalTransforms[boneIndex] = local;
        else
            globalTransforms[boneIndex] = globalTransforms[m_NodeParents[boneIndex]] * local;

        glm::mat4 offset = (boneIndex < (int)m_OffsetMatrices.size()) ? m_OffsetMatrices[boneIndex] : glm::mat4(1.0f);
        finalMatrices.push_back(globalTransforms[boneIndex] * offset);
    }

    return finalMatrices;
}


void ModelAsset::OnLoaded()
{
    // Finalization is handled by the model manager during its finalize pump.
}

uint32_t ModelAsset::GetEmbeddedTextureID(const std::string& path) const
{
    auto it = m_EmbeddedTextures.find(std::pmr::string(path.c_str()));
    if (it != m_EmbeddedTextures.end() && it->second)
    {
        return it->second->GetRendererID();
    }
    return 0;
}

} // namespace CHEngine
