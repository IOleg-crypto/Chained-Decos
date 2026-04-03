#include "assimp_importer.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include <assimp/postprocess.h>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace CHEngine
{
    glm::mat4 AssimpImporter::ConvertMatrix(const aiMatrix4x4& m)
    {
        glm::mat4 res;
        res[0][0] = m.a1; res[1][0] = m.a2; res[2][0] = m.a3; res[3][0] = m.a4;
        res[0][1] = m.b1; res[1][1] = m.b2; res[2][1] = m.b3; res[3][1] = m.b4;
        res[0][2] = m.c1; res[1][2] = m.c2; res[2][2] = m.c3; res[3][2] = m.c4;
        res[0][3] = m.d1; res[1][3] = m.d2; res[2][3] = m.d3; res[3][3] = m.d4;
        return res;
    }

    glm::vec3 AssimpImporter::ConvertVector3(const aiVector3D& v) { return {v.x, v.y, v.z}; }
    glm::quat AssimpImporter::ConvertQuaternion(const aiQuaternion& q) { return {q.w, q.x, q.y, q.z}; }
    glm::vec4 AssimpImporter::ConvertColor(const aiColor4D& c) { return {c.r, c.g, c.b, c.a}; }

    std::string AssimpImporter::ResolveTexturePath(const aiScene* scene, const aiMaterial* aiMat, int type, const std::filesystem::path& modelDir)
    {
        aiString texPath;
        if (aiMat->GetTexture((aiTextureType)type, 0, &texPath) != AI_SUCCESS) return "";

        std::string pathString = texPath.C_Str();
        if (pathString.empty()) return "";

        if (pathString[0] == '*')
        {
            const char* str = pathString.c_str() + 1;
            char* endPtr = nullptr;
            long index = strtol(str, &endPtr, 10);
            if (endPtr != str && index >= 0 && index < (long)scene->mNumTextures)
            {
                aiTexture* tex = scene->mTextures[index];
                std::string ext = (tex->achFormatHint[0] != '\0') ? std::string(".") + tex->achFormatHint : ".png";
                std::string filename = std::string("embedded_") + std::to_string(index) + ext;
                std::filesystem::path texturesDir = modelDir / "textures";
                if (!std::filesystem::exists(texturesDir)) std::filesystem::create_directories(texturesDir);
                std::filesystem::path targetPath = texturesDir / filename;
                if (!std::filesystem::exists(targetPath)) {
                    std::ofstream ofs(targetPath, std::ios::binary);
                    if (ofs) {
                        if (tex->mHeight == 0) ofs.write((const char*)tex->pcData, tex->mWidth);
                        else ofs.write((const char*)tex->pcData, tex->mWidth * tex->mHeight * 4);
                    }
                }
                return targetPath.string();
            }
        }

        std::filesystem::path fullPath = modelDir / pathString;
        if (std::filesystem::exists(fullPath)) return fullPath.string();

        std::string filename = std::filesystem::path(pathString).filename().string();
        std::filesystem::path fallbackPath = modelDir / filename;
        if (std::filesystem::exists(fallbackPath)) return fallbackPath.string();

        return "";
    }

    void AssimpImporter::ProcessHierarchy(aiNode* node, int parent, PendingModelData& data, std::map<aiNode*, int>& nodeToBone)
    {
        int index = (int)data.nodeNames.size();
        nodeToBone[node] = index;
        data.nodeNames.push_back(node->mName.C_Str());
        data.nodeParents.push_back(parent);
        
        glm::mat4 localMat = ConvertMatrix(node->mTransformation);
        data.nodeLocalTransforms.push_back(localMat);

        glm::mat4 globalMat = (parent == -1) ? localMat : data.globalBindPoses[parent] * localMat;
        data.globalBindPoses.push_back(globalMat);

        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            data.instances.push_back({ (int)node->mMeshes[i], globalMat });
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessHierarchy(node->mChildren[i], index, data, nodeToBone);
        }
    }

    void AssimpImporter::ProcessMaterials(const aiScene* scene, const std::filesystem::path& modelDir, PendingModelData& data)
    {
        for (unsigned int i = 0; i < scene->mNumMaterials; i++)
        {
            aiMaterial* aiMat = scene->mMaterials[i];
            RawMaterial mat;
            aiColor4D color;
            if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) mat.albedoColor = ConvertColor(color);
            
            mat.albedoPath = ResolveTexturePath(scene, aiMat, aiTextureType_BASE_COLOR, modelDir);
            if (mat.albedoPath.empty()) mat.albedoPath = ResolveTexturePath(scene, aiMat, aiTextureType_DIFFUSE, modelDir);
            
            mat.normalPath = ResolveTexturePath(scene, aiMat, aiTextureType_NORMALS, modelDir);
            mat.emissivePath = ResolveTexturePath(scene, aiMat, aiTextureType_EMISSIVE, modelDir);
            data.materials.push_back(mat);
        }
    }

    void AssimpImporter::ProcessMeshes(const aiScene* scene, PendingModelData& data)
    {
        data.offsetMatrices.assign(data.nodeNames.size(), glm::mat4(1.0f));

        for (unsigned int m = 0; m < scene->mNumMeshes; m++)
        {
            aiMesh* aiMesh = scene->mMeshes[m];
            RawMesh mesh;
            mesh.materialIndex = aiMesh->mMaterialIndex;

            for (unsigned int v = 0; v < aiMesh->mNumVertices; v++)
            {
                mesh.vertices.push_back(aiMesh->mVertices[v].x);
                mesh.vertices.push_back(aiMesh->mVertices[v].y);
                mesh.vertices.push_back(aiMesh->mVertices[v].z);
                if (aiMesh->HasNormals())
                {
                    mesh.normals.push_back(aiMesh->mNormals[v].x);
                    mesh.normals.push_back(aiMesh->mNormals[v].y);
                    mesh.normals.push_back(aiMesh->mNormals[v].z);
                }
                if (aiMesh->mTextureCoords[0])
                {
                    mesh.texcoords.push_back(aiMesh->mTextureCoords[0][v].x);
                    mesh.texcoords.push_back(aiMesh->mTextureCoords[0][v].y);
                }
                
                mesh.joints.insert(mesh.joints.end(), 4, 0);
                mesh.weights.insert(mesh.weights.end(), 4, 0.0f);
            }

            if (aiMesh->HasBones())
            {
                std::fill(mesh.weights.begin(), mesh.weights.end(), 0.0f);
                std::vector<int> weightCount(aiMesh->mNumVertices, 0);

                for (unsigned int b = 0; b < aiMesh->mNumBones; b++)
                {
                    aiBone* bone = aiMesh->mBones[b];
                    int boneIdx = -1;
                    auto it = std::find(data.nodeNames.begin(), data.nodeNames.end(), bone->mName.C_Str());
                    if (it != data.nodeNames.end()) boneIdx = (int)std::distance(data.nodeNames.begin(), it);

                    if (boneIdx != -1)
                    {
                        data.offsetMatrices[boneIdx] = ConvertMatrix(bone->mOffsetMatrix);
                        for (unsigned int w_idx = 0; w_idx < bone->mNumWeights; w_idx++)
                        {
                            unsigned int vIdx = bone->mWeights[w_idx].mVertexId;
                            if (weightCount[vIdx] < 4)
                            {
                                mesh.joints[vIdx * 4 + weightCount[vIdx]] = (unsigned char)boneIdx;
                                mesh.weights[vIdx * 4 + weightCount[vIdx]] = bone->mWeights[w_idx].mWeight;
                                weightCount[vIdx]++;
                            }
                        }
                    }
                }
            }

            for (unsigned int f = 0; f < aiMesh->mNumFaces; f++)
            {
                for (unsigned int i = 0; i < aiMesh->mFaces[f].mNumIndices; i++)
                    mesh.indices.push_back((unsigned short)aiMesh->mFaces[f].mIndices[i]);
            }
            data.meshes.push_back(std::move(mesh));
        }
    }

    void AssimpImporter::BuildSkeleton(PendingModelData& data)
    {
        data.bones.resize(data.nodeNames.size());
        data.bindPose.resize(data.nodeNames.size());

        for (int i = 0; i < (int)data.nodeNames.size(); i++)
        {
            strncpy(data.bones[i].name, data.nodeNames[i].c_str(), 31);
            data.bones[i].parent = data.nodeParents[i];
            glm::mat4 mat = data.nodeLocalTransforms[i];
            data.bindPose[i].translation = {mat[3][0], mat[3][1], mat[3][2]};
            glm::quat q = glm::quat_cast(mat);
            data.bindPose[i].rotation = {q.x, q.y, q.z, q.w};
            data.bindPose[i].scale = {1, 1, 1}; // Simplified scale
        }
    }

    void AssimpImporter::ProcessAnimations(const aiScene* scene, PendingModelData& data, int samplingFPS)
    {
        for (unsigned int a = 0; a < scene->mNumAnimations; a++)
        {
            aiAnimation* aiAnim = scene->mAnimations[a];
            RawAnimation rawAnim;
            rawAnim.name = aiAnim->mName.C_Str();

            double ticksPerSecond = aiAnim->mTicksPerSecond != 0 ? aiAnim->mTicksPerSecond : 25.0;
            double durationInSeconds = aiAnim->mDuration / ticksPerSecond;
            int frameCount = (int)(durationInSeconds * (double)samplingFPS) + 1;
            int boneCount = (int)data.nodeNames.size();

            rawAnim.frameRate = (float)samplingFPS;
            rawAnim.frameCount = frameCount;
            rawAnim.boneCount = boneCount;
            rawAnim.framePoses.resize(frameCount * boneCount);

            for (int f = 0; f < frameCount; f++)
            {
                // Animation sampling logic here (omitted for brevity in initial refactor, using bind pose for now)
                for (int i = 0; i < boneCount; i++) rawAnim.framePoses[f * boneCount + i] = data.bindPose[i];
            }
            data.animations.push_back(std::move(rawAnim));
        }
    }

    PendingModelData AssimpImporter::Import(const std::filesystem::path& path, int samplingFPS)
    {
        PendingModelData data{};
        std::string pathStr = path.generic_string();
        
        if (!std::filesystem::exists(path)) {
            CH_CORE_ERROR("AssimpImporter: File does not exist: {}", pathStr);
            return data;
        }

        uintmax_t fileSize = std::filesystem::file_size(path);
        CH_CORE_INFO("AssimpImporter: Opening: {} ({} bytes)", pathStr, fileSize);

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            CH_CORE_ERROR("AssimpImporter: Failed to open file: {}", pathStr);
            return data;
        }

        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), fileSize);
        file.close();

        Assimp::Importer importer;
        // Simple flags for better stability
        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | 
                             aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights |
                             aiProcess_ValidateDataStructure | aiProcess_SortByPType;

        importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);

        const aiScene* scene = importer.ReadFileFromMemory(buffer.data(), buffer.size(), flags, pathStr.c_str());
        if (!scene || !scene->mRootNode) {
            CH_CORE_ERROR("Assimp Error: {}", importer.GetErrorString());
            return data;
        }

        std::map<aiNode*, int> nodeToBone;
        ProcessHierarchy(scene->mRootNode, -1, data, nodeToBone);
        ProcessMaterials(scene, path.parent_path(), data);
        ProcessMeshes(scene, data);
        BuildSkeleton(data);
        ProcessAnimations(scene, data, samplingFPS);

        data.isValid = true;
        return data;
    }
}
