#ifndef CH_ASSIMP_IMPORTER_H
#define CH_ASSIMP_IMPORTER_H

#include "engine/graphics/api/model_data.h"
#include <filesystem>
#include <unordered_map>
#include <vector>

struct aiScene;
struct aiNode;

namespace Chained
{
class AssimpImporter
{
public:
    static PendingModelData Import(const std::filesystem::path& path, int samplingFPS = 30);

private:
    AssimpImporter(const std::filesystem::path& path, int samplingFPS, const aiScene* scene);
    PendingModelData Execute();

    void ProcessNode(aiNode* node, int parentIdx);
    void ProcessSingleMesh(uint32_t meshIndex);
    void ProcessMeshes();
    void ProcessMaterials();
    void DecodeEmbeddedTextures();
    void ProcessAnimations();
    void MergeMeshesByMaterial();

private:
    std::filesystem::path m_Path;
    std::filesystem::path m_ModelDir;
    int m_SamplingFPS;
    const aiScene* m_Scene;

    PendingModelData m_Data;
    std::unordered_map<std::string, int> m_NameToIndex;
    std::vector<std::vector<std::pair<int, glm::mat4>>> m_MeshOffsetWrites;
};
} // namespace Chained
#endif