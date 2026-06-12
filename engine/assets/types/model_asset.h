#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/model_data.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <memory_resource>
#include <string>
#include <unordered_map>
#include <vector>

namespace Chained
{
class Texture;

class ModelAsset : public Asset
{
public:
    ModelAsset()
        : Asset(GetStaticType())
    {
    }
    ModelAsset(AssetHandle handle)
        : Asset(GetStaticType(), handle)
    {
    }
    virtual ~ModelAsset() = default;

    friend class SceneRenderer;

    static AssetType GetStaticType()
    {
        return AssetType::Model;
    }

    void OnLoaded();

    size_t GetMemoryUsage() const override
    {
        return 0; /* To be calculated properly */
    }

    const Model& GetModel() const
    {
        return m_Model;
    }
    const std::vector<RawAnimation>& GetAnimations() const
    {
        return m_Animations;
    }
    const std::vector<MeshInstance>& GetInstances() const
    {
        return m_Instances;
    }
    const BoundingBox& GetBoundingBox() const
    {
        return m_BoundingBox;
    }

    const std::vector<RawMesh>& GetRawMeshes() const
    {
        return m_RawMeshes;
    }

    // Helpers
    int GetAnimationCount() const;

    std::string GetAnimationName(int index) const;
    std::vector<glm::mat4> GetBoneMatrices(int animationIndex, int frame) const;

    const std::vector<Material>& GetMaterials() const
    {
        return m_Materials;
    }

    std::vector<Material>& GetMaterials()
    {
        return m_Materials;
    }

    // Get renderer ID for embedded texture (e.g., "*0", "*1").
    uint32_t GetEmbeddedTextureID(const std::string& path) const;

private:
    void SetPendingData(PendingModelData&& data)
    {
        m_PendingData = std::move(data);
        m_HasPendingData = true;
    }

    void SetModel(const Model& model)
    {
        m_Model = model;
    }
    void SetAnimations(const std::vector<RawAnimation>& animations)
    {
        m_Animations = animations;
    }
    void SetInstances(const std::vector<MeshInstance>& instances)
    {
        m_Instances = instances;
    }
    void SetBoundingBox(const BoundingBox& bbox)
    {
        m_BoundingBox = bbox;
    }
    void SetRawMeshes(const std::vector<RawMesh>& meshes)
    {
        m_RawMeshes = meshes;
    }
    void SetOffsetMatrices(const std::vector<glm::mat4>& matrices)
    {
        m_OffsetMatrices = matrices;
    }
    void SetNodeNames(const std::vector<std::string>& names)
    {
        m_NodeNames = names;
    }
    void SetNodeParents(const std::vector<int>& parents)
    {
        m_NodeParents = parents;
    }

    Model m_Model;
    std::vector<RawMesh> m_RawMeshes;
    std::vector<RawAnimation> m_Animations;
    std::vector<MeshInstance> m_Instances;
    std::vector<Material> m_Materials;
    BoundingBox m_BoundingBox = {{0, 0, 0}, {0, 0, 0}};

public: // Internal data for loader
    std::vector<glm::mat4> m_OffsetMatrices;
    std::vector<std::string> m_NodeNames;
    std::vector<int> m_NodeParents;

    // Loading data
    PendingModelData m_PendingData;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_EmbeddedTextures;
    bool m_HasPendingData = false;
};

inline int ModelAsset::GetAnimationCount() const
{
    return (int)m_Animations.size();
}
} // namespace Chained

#endif // CH_MODEL_ASSET_H
