#include "mesh_importer.h"
#include "engine/core/log.h"
#include "model_asset.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <filesystem>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>

namespace CHEngine
{
// Matrix conversion: Assimp (Row-Major) to Raylib (Column-Major storage)
Matrix MeshImporter::ConvertMatrix(const aiMatrix4x4& m)
{
    Matrix res;
    res.m0 = m.a1;
    res.m4 = m.a2;
    res.m8 = m.a3;
    res.m12 = m.a4;
    res.m1 = m.b1;
    res.m5 = m.b2;
    res.m9 = m.b3;
    res.m13 = m.b4;
    res.m2 = m.c1;
    res.m6 = m.c2;
    res.m10 = m.c3;
    res.m14 = m.c4;
    res.m3 = m.d1;
    res.m7 = m.d2;
    res.m11 = m.d3;
    res.m15 = m.d4;
    return res;
}

Vector3 MeshImporter::ConvertVector3(const aiVector3D& v)
{
    return {v.x, v.y, v.z};
}
Quaternion MeshImporter::ConvertQuaternion(const aiQuaternion& q)
{
    return {q.x, q.y, q.z, q.w};
}
Color MeshImporter::ConvertColor(const aiColor4D& c)
{
    return {(unsigned char)(c.r * 255), (unsigned char)(c.g * 255), (unsigned char)(c.b * 255),
            (unsigned char)(c.a * 255)};
}

// --- Helper Functions for LoadMeshDataFromDisk ---

void MeshImporter::ProcessHierarchy(aiNode* node, int parent, PendingModelData& data,
                                    std::map<aiNode*, int>& nodeToBone, std::vector<int>& meshToNode)
{
    int index = (int)data.nodeNames.size();
    nodeToBone[node] = index;
    data.nodeNames.push_back(node->mName.C_Str());
    data.nodeParents.push_back(parent);
    data.nodeLocalTransforms.push_back(ConvertMatrix(node->mTransformation));

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        meshToNode[node->mMeshes[i]] = index;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessHierarchy(node->mChildren[i], index, data, nodeToBone, meshToNode);
    }
}

void MeshImporter::ProcessMaterials(const aiScene* scene, const std::filesystem::path& modelDir, PendingModelData& data)
{
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* aiMat = scene->mMaterials[i];
        RawMaterial mat;
        aiColor4D color;
        if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        {
            mat.albedoColor = ConvertColor(color);
        }
        mat.albedoPath = ResolveTexturePath(scene, aiMat, aiTextureType_BASE_COLOR, modelDir);
        if (mat.albedoPath.empty())
        {
            mat.albedoPath = ResolveTexturePath(scene, aiMat, aiTextureType_DIFFUSE, modelDir);
        }
        mat.normalPath = ResolveTexturePath(scene, aiMat, aiTextureType_NORMALS, modelDir);
        mat.emissivePath = ResolveTexturePath(scene, aiMat, aiTextureType_EMISSIVE, modelDir);
        data.materials.push_back(mat);
    }
}

void MeshImporter::ProcessMeshes(const aiScene* scene, const std::vector<int>& meshToNode, PendingModelData& data)
{
    data.offsetMatrices.assign(data.nodeNames.size(), MatrixIdentity());

    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        aiMesh* aiMesh = scene->mMeshes[m];
        RawMesh mesh;
        mesh.materialIndex = aiMesh->mMaterialIndex;

        int owningNodeIdx = meshToNode[m];

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
            mesh.joints.insert(mesh.joints.end(), 4, (unsigned char)(owningNodeIdx != -1 ? owningNodeIdx : 0));
            mesh.weights.insert(mesh.weights.end(), 4, 0.0f);
            mesh.weights[mesh.weights.size() - 4] = 1.0f;
        }

        if (owningNodeIdx != -1 && !aiMesh->HasBones())
        {
            data.offsetMatrices[owningNodeIdx] = MatrixIdentity();
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
                if (it != data.nodeNames.end())
                {
                    boneIdx = (int)std::distance(data.nodeNames.begin(), it);
                }

                if (boneIdx != -1)
                {
                    data.offsetMatrices[boneIdx] = ConvertMatrix(bone->mOffsetMatrix);
                    for (unsigned int w = 0; w < bone->mNumWeights; w++)
                    {
                        unsigned int vIdx = bone->mWeights[w].mVertexId;
                        if (weightCount[vIdx] < 4)
                        {
                            mesh.joints[vIdx * 4 + weightCount[vIdx]] = (unsigned char)boneIdx;
                            mesh.weights[vIdx * 4 + weightCount[vIdx]] = bone->mWeights[w].mWeight;
                            weightCount[vIdx]++;
                        }
                    }
                }
            }

            for (unsigned int v = 0; v < aiMesh->mNumVertices; v++)
            {
                float totalWeight = 0.0f;
                for (int i = 0; i < 4; i++)
                {
                    totalWeight += mesh.weights[v * 4 + i];
                }

                if (totalWeight <= 0.001f)
                {
                    int fallbackBone = (owningNodeIdx != -1) ? owningNodeIdx : 0;
                    mesh.joints[v * 4] = (unsigned char)fallbackBone;
                    mesh.weights[v * 4] = 1.0f;
                    totalWeight = 1.0f;
                }

                if (totalWeight > 0.0f)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        mesh.weights[v * 4 + i] /= totalWeight;
                    }
                }
            }
        }

        for (unsigned int f = 0; f < aiMesh->mNumFaces; f++)
        {
            for (unsigned int i = 0; i < aiMesh->mFaces[f].mNumIndices; i++)
            {
                mesh.indices.push_back((unsigned short)aiMesh->mFaces[f].mIndices[i]);
            }
        }
        data.meshes.push_back(std::move(mesh));
    }
}

void MeshImporter::BuildSkeleton(PendingModelData& data)
{
    data.bones.resize(data.nodeNames.size());
    data.bindPose.resize(data.nodeNames.size());

    for (int i = 0; i < (int)data.nodeNames.size(); i++)
    {
        strncpy(data.bones[i].name, data.nodeNames[i].c_str(), 31);
        data.bones[i].parent = data.nodeParents[i];

        Matrix mat = data.nodeLocalTransforms[i];
        data.bindPose[i].translation = {mat.m12, mat.m13, mat.m14};
        data.bindPose[i].rotation = QuaternionFromMatrix(mat);
        data.bindPose[i].scale = {Vector3Length({mat.m0, mat.m1, mat.m2}), Vector3Length({mat.m4, mat.m5, mat.m6}),
                                  Vector3Length({mat.m8, mat.m9, mat.m10})};
    }
}

void MeshImporter::ProcessAnimations(const aiScene* scene, PendingModelData& data, int samplingFPS)
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

        rawAnim.frameCount = frameCount;
        rawAnim.boneCount = boneCount;
        rawAnim.framePoses.resize(frameCount * boneCount);

        std::vector<aiNodeAnim*> boneToChannel(boneCount, nullptr);
        for (unsigned int c = 0; c < aiAnim->mNumChannels; c++)
        {
            aiNodeAnim* channel = aiAnim->mChannels[c];
            for (int i = 0; i < boneCount; i++)
            {
                if (channel->mNodeName == aiString(data.nodeNames[i]))
                {
                    boneToChannel[i] = channel;
                    break;
                }
            }
        }

        for (int f = 0; f < frameCount; f++)
        {
            double timeInTicks = (double)f / (double)samplingFPS * ticksPerSecond;
            for (int i = 0; i < boneCount; i++)
            {
                aiNodeAnim* channel = boneToChannel[i];
                if (channel)
                {
                    aiVector3D pos = channel->mPositionKeys[0].mValue;
                    if (channel->mNumPositionKeys > 1)
                    {
                        for (unsigned int k = 0; k < channel->mNumPositionKeys - 1; k++)
                        {
                            if (timeInTicks < channel->mPositionKeys[k + 1].mTime)
                            {
                                float factor =
                                    (float)((timeInTicks - channel->mPositionKeys[k].mTime) /
                                            (channel->mPositionKeys[k + 1].mTime - channel->mPositionKeys[k].mTime));
                                pos =
                                    channel->mPositionKeys[k].mValue +
                                    (channel->mPositionKeys[k + 1].mValue - channel->mPositionKeys[k].mValue) * factor;
                                break;
                            }
                        }
                    }
                    aiQuaternion rot = channel->mRotationKeys[0].mValue;
                    if (channel->mNumRotationKeys > 1)
                    {
                        for (unsigned int k = 0; k < channel->mNumRotationKeys - 1; k++)
                        {
                            if (timeInTicks < channel->mRotationKeys[k + 1].mTime)
                            {
                                float factor =
                                    (float)((timeInTicks - channel->mRotationKeys[k].mTime) /
                                            (channel->mRotationKeys[k + 1].mTime - channel->mRotationKeys[k].mTime));
                                aiQuaternion::Interpolate(rot, channel->mRotationKeys[k].mValue,
                                                          channel->mRotationKeys[k + 1].mValue, factor);
                                break;
                            }
                        }
                    }
                    aiVector3D scl = {1, 1, 1};
                    if (channel->mNumScalingKeys > 0)
                    {
                        scl = channel->mScalingKeys[0].mValue;
                        if (channel->mNumScalingKeys > 1)
                        {
                            for (unsigned int k = 0; k < channel->mNumScalingKeys - 1; k++)
                            {
                                if (timeInTicks < channel->mScalingKeys[k + 1].mTime)
                                {
                                    float factor =
                                        (float)((timeInTicks - channel->mScalingKeys[k].mTime) /
                                                (channel->mScalingKeys[k + 1].mTime - channel->mScalingKeys[k].mTime));
                                    scl = channel->mScalingKeys[k].mValue +
                                          (channel->mScalingKeys[k + 1].mValue - channel->mScalingKeys[k].mValue) *
                                              factor;
                                    break;
                                }
                            }
                        }
                    }
                    rawAnim.framePoses[f * boneCount + i] = {ConvertVector3(pos), ConvertQuaternion(rot),
                                                             ConvertVector3(scl)};
                }
                else
                {
                    rawAnim.framePoses[f * boneCount + i] = data.bindPose[i];
                }
            }
        }
        data.animations.push_back(std::move(rawAnim));
    }
}

std::string MeshImporter::ResolveTexturePath(const aiScene* scene, const aiMaterial* aiMat, aiTextureType type,
                                             const std::filesystem::path& modelDir)
{
    aiString texPath;
    if (aiMat->GetTexture(type, 0, &texPath) != AI_SUCCESS)
    {
        return "";
    }

    std::string pathString = texPath.C_Str();
    if (pathString.empty())
    {
        return "";
    }

    // Handle embedded textures (starts with *)
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
            if (!std::filesystem::exists(texturesDir))
            {
                std::filesystem::create_directories(texturesDir);
            }

            std::filesystem::path targetPath = texturesDir / filename;

            if (!std::filesystem::exists(targetPath))
            {
                std::ofstream ofs(targetPath, std::ios::binary);
                if (ofs)
                {
                    if (tex->mHeight == 0)
                    {
                        ofs.write((const char*)tex->pcData, tex->mWidth);
                    }
                    else
                    {
                        ofs.write((const char*)tex->pcData, tex->mWidth * tex->mHeight * 4);
                    }
                }
            }
            return targetPath.string();
        }
    }

    std::filesystem::path fullPath = modelDir / pathString;
    if (std::filesystem::exists(fullPath))
    {
        return fullPath.string();
    }

    // Fallback: search for the filename in the model directory
    std::string filename = std::filesystem::path(pathString).filename().string();
    std::filesystem::path fallbackPath = modelDir / filename;
    if (std::filesystem::exists(fallbackPath))
    {
        CH_CORE_WARN("MeshImporter: Texture {} not found at specified path, but found in model directory: {}",
                     pathString, fallbackPath.string());
        return fallbackPath.string();
    }

    CH_CORE_ERROR("MeshImporter: Could not resolve texture path: {} (Searched in: {})", pathString, modelDir.string());
    return "";
}

PendingModelData MeshImporter::LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS)
{
    PendingModelData data{};
    data.fullPath = std::filesystem::absolute(path).string();
    std::filesystem::path modelDir = path.parent_path();

    Assimp::Importer importer;

    // Configure to remove non-triangle primitives (points/lines)
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    // CRITICAL: Set vertex limit for SplitLargeMeshes to match Raylib's 16-bit indices (unsigned short)
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);

    const aiScene* scene = importer.ReadFile(
        path.string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
                           aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices |
                           aiProcess_PopulateArmatureData | aiProcess_FlipUVs | aiProcess_ValidateDataStructure |
                           // aiProcess_GlobalScale | // Removed: can cause issues with animations and manual scaling
                           aiProcess_SortByPType |          // Needed for point/line removal
                           aiProcess_SplitLargeMeshes |     // Critical for 16-bit index systems like Raylib
                           aiProcess_ImproveCacheLocality | // Performance optimization
                           aiProcess_RemoveRedundantMaterials |
                           aiProcess_FindInstances |   // Optimize: find identical meshes
                           aiProcess_FindInvalidData | // Clean up the import
                           aiProcess_OptimizeGraph |   // Simplify hierarchy
                           aiProcess_OptimizeMeshes    // Combine small meshes where possible
    );

    if (!scene || !scene->mRootNode)
    {
        CH_CORE_ERROR("Assimp Importer: {}", importer.GetErrorString());
        return data;
    }

    // 1. Hierarchy & Mesh-to-Node Mapping
    std::map<aiNode*, int> nodeToBone;
    data.meshToNode.assign(scene->mNumMeshes, -1);
    ProcessHierarchy(scene->mRootNode, -1, data, nodeToBone, data.meshToNode);

    // 2. Compute Global Bind Pose Transforms
    data.globalBindPoses.resize(data.nodeNames.size());
    for (size_t i = 0; i < data.nodeNames.size(); i++)
    {
        int parent = data.nodeParents[i];
        if (parent == -1)
        {
            data.globalBindPoses[i] = data.nodeLocalTransforms[i];
        }
        else
        {
            data.globalBindPoses[i] = MatrixMultiply(data.globalBindPoses[parent], data.nodeLocalTransforms[i]);
        }
    }

    // 3. Sequential Processing
    ProcessMaterials(scene, modelDir, data);
    ProcessMeshes(scene, data.meshToNode, data);
    BuildSkeleton(data);
    ProcessAnimations(scene, data, samplingFPS);

    data.isValid = true;
    return data;
}

std::shared_ptr<ModelAsset> MeshImporter::ImportMesh(const std::filesystem::path& path)
{
    std::string pathStr = path.string();
    if (pathStr.starts_with(":"))
    {
        auto asset = std::make_shared<ModelAsset>();
        asset->SetPath(pathStr);
        asset->GetModel() = GenerateProceduralModel(pathStr);
        asset->SetState(AssetState::Ready);
        return asset;
    }

    CH_CORE_INFO("MeshImporter: Importing via Assimp: {}", pathStr);
    auto asset = std::make_shared<ModelAsset>();
    asset->SetPath(pathStr);

    auto pendingData = LoadMeshDataFromDisk(path);
    if (pendingData.isValid)
    {
        asset->SetPendingData(pendingData);
        asset->UploadToGPU();
    }
    else
    {
        asset->SetState(AssetState::Failed);
    }

    return asset;
}

Model MeshImporter::GenerateProceduralModel(const std::string& type)
{
    Mesh mesh = {0};
    if (type == ":cube:")
    {
        mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    }
    else if (type == ":sphere:")
    {
        mesh = GenMeshSphere(0.5f, 16, 16);
    }
    else if (type == ":plane:")
    {
        mesh = GenMeshPlane(10.0f, 10.0f, 10, 10);
    }
    else if (type == ":torus:")
    {
        mesh = GenMeshTorus(0.2f, 0.4f, 16, 16);
    }
    else if (type == ":cylinder:")
    {
        mesh = GenMeshCylinder(0.5f, 1.0f, 16);
    }
    else if (type == ":cone:")
    {
        mesh = GenMeshCone(0.5f, 1.0f, 16);
    }
    else if (type == ":knot:")
    {
        mesh = GenMeshKnot(0.5f, 0.2f, 16, 128);
    }
    else if (type == ":hemisphere:")
    {
        mesh = GenMeshHemiSphere(0.5f, 16, 16);
    }

    if (mesh.vertexCount == 0)
    {
        return {0};
    }
    return LoadModelFromMesh(mesh);
}
} // namespace CHEngine
