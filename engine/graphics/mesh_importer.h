#ifndef CH_MESH_IMPORTER_H
#define CH_MESH_IMPORTER_H

#include "asset_importer.h"
#include "model_data.h"
#include <memory>
#include <map>
#include <vector>
#include <filesystem>

#include <assimp/scene.h>

namespace CHEngine
{
    class ModelAsset;
    
    class MeshImporter : public AssetImporter
    {
    public:
        static std::shared_ptr<ModelAsset> ImportMesh(const std::filesystem::path& path);
        
        // For procedural mesh generation
        static Model GenerateProceduralModel(const std::string& type);

        // For async loading
        static PendingModelData LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS = 30);

    private:
        static void ProcessHierarchy(aiNode* node, int parent, PendingModelData& data, std::map<aiNode*, int>& nodeToBone, std::vector<int>& meshToNode);
        static void ProcessMaterials(const aiScene* scene, const std::filesystem::path& modelDir, PendingModelData& data);
        static void ProcessMeshes(const aiScene* scene, const std::vector<int>& meshToNode, PendingModelData& data);
        static void BuildSkeleton(PendingModelData& data);
        static void ProcessAnimations(const aiScene* scene, PendingModelData& data, int samplingFPS);
        static std::string ResolveTexturePath(const aiScene* scene, const aiMaterial* aiMat, aiTextureType type, const std::filesystem::path& modelDir);

        // Conversion helpers
        static Matrix ConvertMatrix(const aiMatrix4x4& m);
        static Vector3 ConvertVector3(const aiVector3D& v);
        static Quaternion ConvertQuaternion(const aiQuaternion& q);
        static Color ConvertColor(const aiColor4D& c);
    };
}

#endif // CH_MESH_IMPORTER_H
