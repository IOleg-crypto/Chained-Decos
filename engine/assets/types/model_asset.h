#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "engine/assets/asset.h"
#include "engine/graphics/api/model_data.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory_resource>

namespace CHEngine
{
class Texture;

    //
    //
    // struct CH_API Ray
    // {
    //     glm::vec3 position;
    //     glm::vec3 direction;
    // };

class ModelAsset : public Asset, public std::enable_shared_from_this<ModelAsset>
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

    void OnLoaded() override;

    const Model& GetModel() const
    {
        return m_Model;
    }
    const std::pmr::vector<RawAnimation>& GetAnimations() const
    {
        return m_Animations;
    }
    const std::pmr::vector<MeshInstance>& GetInstances() const
    {
        return m_Instances;
    }
    const BoundingBox& GetBoundingBox() const
    {
        return m_BoundingBox;
    }

    const std::pmr::vector<RawMesh>& GetRawMeshes() const
    {
        return m_RawMeshes;
    }

    // Helpers
    int GetAnimationCount() const
    {
        return (int)m_Animations.size();
    }
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
    
    void SetModel(const Model& model) { m_Model = model; }
    void SetAnimations(const std::pmr::vector<RawAnimation>& animations) { m_Animations = animations; }
    void SetInstances(const std::pmr::vector<MeshInstance>& instances) { m_Instances = instances; }
    void SetBoundingBox(const BoundingBox& bbox) { m_BoundingBox = bbox; }
    void SetRawMeshes(const std::pmr::vector<RawMesh>& meshes) { m_RawMeshes = meshes; }
    void SetOffsetMatrices(const std::pmr::vector<glm::mat4>& matrices) { m_OffsetMatrices = matrices; }
    void SetNodeNames(const std::pmr::vector<std::pmr::string>& names) { m_NodeNames = names; }
    void SetNodeParents(const std::pmr::vector<int>& parents) { m_NodeParents = parents; }
    Model m_Model;
    std::pmr::vector<RawMesh> m_RawMeshes;
    std::pmr::vector<RawAnimation> m_Animations;
    std::pmr::vector<MeshInstance> m_Instances;
    std::vector<Material> m_Materials;
    BoundingBox m_BoundingBox = {{0, 0, 0}, {0, 0, 0}};

public: // Internal data for loader
    std::pmr::vector<glm::mat4> m_OffsetMatrices;
    std::pmr::vector<std::pmr::string> m_NodeNames;
    std::pmr::vector<int> m_NodeParents;

    // Loading data
    PendingModelData m_PendingData;
    std::pmr::unordered_map<std::pmr::string, std::shared_ptr<Texture>> m_EmbeddedTextures;
    bool m_HasPendingData = false;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
