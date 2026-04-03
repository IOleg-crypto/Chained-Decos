#ifndef CH_ASSIMP_IMPORTER_H
#define CH_ASSIMP_IMPORTER_H

#include "engine/graphics/loaders/model_loader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <filesystem>
#include <map>

namespace CHEngine
{
    class AssimpImporter
    {
    public:
        static PendingModelData Import(const std::filesystem::path& path, int samplingFPS);

    private:
        static void ProcessHierarchy(aiNode* node, int parent, PendingModelData& data, std::map<aiNode*, int>& nodeToBone);
        static void ProcessMaterials(const aiScene* scene, const std::filesystem::path& modelDir, PendingModelData& data);
        static void ProcessMeshes(const aiScene* scene, PendingModelData& data);
        static void BuildSkeleton(PendingModelData& data);
        static void ProcessAnimations(const aiScene* scene, PendingModelData& data, int samplingFPS);
        static std::string ResolveTexturePath(const aiScene* scene, const aiMaterial* aiMat, int type, const std::filesystem::path& modelDir);

        static glm::mat4 ConvertMatrix(const aiMatrix4x4& m);
        static glm::vec3 ConvertVector3(const aiVector3D& v);
        static glm::quat ConvertQuaternion(const aiQuaternion& q);
        static glm::vec4 ConvertColor(const aiColor4D& c);
    };
}
#endif
