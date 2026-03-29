#ifndef CH_MODEL_ASSET_H
#define CH_MODEL_ASSET_H

#include "engine/core/assets/asset.h"
#include "engine/core/base.h"
#include "engine/graphics/api/model_data.h"
#include "engine/graphics/pipeline/renderer_types.h"
#include <future>
// #include <mutex>
#include <string>
#include <vector>

namespace CHEngine
{
class ModelAsset : public Asset
{
public:
    ModelAsset()
        : Asset(GetStaticType())
    {
    }
    virtual ~ModelAsset() = default;

    static AssetType GetStaticType()
    {
        return AssetType::Model;
    }

    void OnLoaded() override;

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
    int GetAnimationCount() const
    {
        return (int)m_Animations.size();
    }
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
    std::string GetAnimationName(int index) const;

private:
    Model m_Model;
    std::vector<RawMesh> m_RawMeshes;
    std::vector<RawAnimation> m_Animations;
    std::vector<MeshInstance> m_Instances;
    BoundingBox m_BoundingBox = {{0, 0, 0}, {0, 0, 0}};

    // Skeleton data
    std::vector<glm::mat4> m_OffsetMatrices;
    std::vector<std::string> m_NodeNames;
    std::vector<int> m_NodeParents;

    // Loading data
    PendingModelData m_PendingData;
    bool m_HasPendingData = false;
};
} // namespace CHEngine

#endif // CH_MODEL_ASSET_H
