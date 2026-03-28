#include "engine/graphics/importers/mesh_importer.h"
#include "engine/core/log.h"
#include "engine/scene/project.h"
#include "engine/graphics/assets/model_asset.h"
#include <algorithm>
#include <filesystem>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace CHEngine
{
// Matrix conversion: Assimp (Row-Major) to GLM (Column-Major storage by default)

glm::mat4 MeshImporter::ConvertMatrix(const aiMatrix4x4& m)
{
    glm::mat4 res;
    res[0][0] = m.a1; res[1][0] = m.a2; res[2][0] = m.a3; res[3][0] = m.a4;
    res[0][1] = m.b1; res[1][1] = m.b2; res[2][1] = m.b3; res[3][1] = m.b4;
    res[0][2] = m.c1; res[1][2] = m.c2; res[2][2] = m.c3; res[3][2] = m.c4;
    res[0][3] = m.d1; res[1][3] = m.d2; res[2][3] = m.d3; res[3][3] = m.d4;
    return res;
}

glm::vec3 MeshImporter::ConvertVector3(const aiVector3D& v)
{
    return {v.x, v.y, v.z};
}
glm::quat MeshImporter::ConvertQuaternion(const aiQuaternion& q)
{
    return {q.w, q.x, q.y, q.z}; // Assimp quat is (w, x, y, z), GLM quat is also (w, x, y, z) in constructor
}
glm::vec4 MeshImporter::ConvertColor(const aiColor4D& c)
{
    return {c.r, c.g, c.b, c.a};
}

// --- Helper Functions for LoadMeshDataFromDisk ---

void MeshImporter::ProcessHierarchy(aiNode* node, int parent, PendingModelData& data,
                                    std::map<aiNode*, int>& nodeToBone)
{
    int index = (int)data.nodeNames.size();
    nodeToBone[node] = index;
    data.nodeNames.push_back(node->mName.C_Str());
    data.nodeParents.push_back(parent);
    
    glm::mat4 localMat = ConvertMatrix(node->mTransformation);
    data.nodeLocalTransforms.push_back(localMat);

    // Compute Global Transform (GLM uses column-major multiplication: Global = ParentGlobal * Local)
    glm::mat4 globalMat = (parent == -1) ? localMat : data.globalBindPoses[parent] * localMat;
    data.globalBindPoses.push_back(globalMat);

    // Create flat instances for rendering and AABB
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        data.instances.push_back({ (int)node->mMeshes[i], globalMat });
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessHierarchy(node->mChildren[i], index, data, nodeToBone);
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

void MeshImporter::ProcessMeshes(const aiScene* scene, PendingModelData& data)
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
            if (aiMesh->mTangents)
            {
                mesh.tangents.push_back(aiMesh->mTangents[v].x);
                mesh.tangents.push_back(aiMesh->mTangents[v].y);
                mesh.tangents.push_back(aiMesh->mTangents[v].z);

                // Calculate handedness (w component) for tangents
                if (aiMesh->mBitangents && aiMesh->HasNormals())

                {
                    glm::vec3 n = { aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z };
                    glm::vec3 t = { aiMesh->mTangents[v].x, aiMesh->mTangents[v].y, aiMesh->mTangents[v].z };
                    glm::vec3 b = { aiMesh->mBitangents[v].x, aiMesh->mBitangents[v].y, aiMesh->mBitangents[v].z };

                    float w = (glm::dot(glm::cross(n, t), b) < 0.0f) ? -1.0f : 1.0f;
                    mesh.tangents.push_back(w);
                }
                else
                {
                    mesh.tangents.push_back(1.0f);
                }
            }
            
            // Initialize skinning data as empty (no bone influence by default)
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
                    mesh.joints[v * 4] = 0;
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

        glm::mat4 mat = data.nodeLocalTransforms[i];
        
        // Extract translation
        data.bindPose[i].translation = {mat[3][0], mat[3][1], mat[3][2]};
        
        // Extract rotation (using GLM to convert matrix to quat)
        glm::quat q = glm::quat_cast(mat);
        data.bindPose[i].rotation = {q.x, q.y, q.z, q.w};
        
        // Extract scale
        data.bindPose[i].scale = {
            glm::length(glm::vec3(mat[0][0], mat[0][1], mat[0][2])),
            glm::length(glm::vec3(mat[1][0], mat[1][1], mat[1][2])),
            glm::length(glm::vec3(mat[2][0], mat[2][1], mat[2][2]))
        };
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

        // Store the sampling frame rate (the rate at which we resampled the animation during import)
        rawAnim.frameRate = (float)samplingFPS;
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

    // Load settings from project
    bool calculateTangents = true;
    bool flipUVs = true;
    bool importMaterials = true;

    if (auto project = Project::GetActive())
    {
        const auto& meshSettings = project->GetConfig().Mesh;
        calculateTangents = meshSettings.CalculateTangents;
        flipUVs = meshSettings.FlipUVs;
        importMaterials = meshSettings.ImportMaterials;
    }

    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                         aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices |
                         aiProcess_ValidateDataStructure |
                         aiProcess_SortByPType |          // Needed for point/line removal
                         aiProcess_SplitLargeMeshes |     // Critical for 16-bit index systems like Raylib
                         aiProcess_ImproveCacheLocality | // Performance optimization
                         aiProcess_RemoveRedundantMaterials |
                         aiProcess_FindInstances |   // Optimize: find identical meshes
                         aiProcess_FindInvalidData | // Clean up the import
                         aiProcess_OptimizeGraph |   // Simplify hierarchy
                         aiProcess_OptimizeMeshes;   // Combine small meshes where possible

    if (calculateTangents) flags |= aiProcess_CalcTangentSpace;
    if (flipUVs) flags |= aiProcess_FlipUVs;

    const aiScene* scene = importer.ReadFile(path.string(), flags);

    if (!scene || !scene->mRootNode)
    {
        CH_CORE_ERROR("Assimp Importer: {}", importer.GetErrorString());
        return data;
    }

    // 1. Hierarchy & Instances
    std::map<aiNode*, int> nodeToBone;
    data.globalBindPoses.clear(); // Will be populated by ProcessHierarchy
    ProcessHierarchy(scene->mRootNode, -1, data, nodeToBone);

    // 3. Sequential Processing
    if (importMaterials)
    {
        ProcessMaterials(scene, modelDir, data);
    }
    else
    {
        // Provide a default material
        data.materials.push_back(RawMaterial{});
    }
    
    ProcessMeshes(scene, data);
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

Model MeshImporter::GenerateProceduralModel(const std::string& type, const ProceduralParameters& params)
{
    Model model;
    RawMesh raw;

    if (type == ":cube")
    {
        float s = params.Dimensions.x * 0.5f;
        raw.vertices = {
            -s,-s, s,  s,-s, s,  s, s, s, -s, s, s, // Front
            -s,-s,-s, -s, s,-s,  s, s,-s,  s,-s,-s, // Back
            -s, s,-s, -s, s, s,  s, s, s,  s, s,-s, // Top
            -s,-s,-s,  s,-s,-s,  s,-s, s, -s,-s, s, // Bottom
             s,-s,-s,  s, s,-s,  s, s, s,  s,-s, s, // Right
            -s,-s,-s, -s,-s, s, -s, s, s, -s, s,-s  // Left
        };
        raw.normals = {
             0, 0, 1,  0, 0, 1,  0, 0, 1,  0, 0, 1,
             0, 0,-1,  0, 0,-1,  0, 0,-1,  0, 0,-1,
             0, 1, 0,  0, 1, 0,  0, 1, 0,  0, 1, 0,
             0,-1, 0,  0,-1, 0,  0,-1, 0,  0,-1, 0,
             1, 0, 0,  1, 0, 0,  1, 0, 0,  1, 0, 0,
            -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0
        };
        raw.indices = {
            0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4,
            8, 9,10,10,11, 8, 12,13,14,14,15,12,
            16,17,18,18,19,16, 20,21,22,22,23,20
        };
    }
    else if (type == ":sphere")
    {
        int sectors = 32;
        int stacks = 16;
        float radius = params.Radius;
        float sectorStep = 2 * glm::pi<float>() / sectors;
        float stackStep = glm::pi<float>() / stacks;

        for (int i = 0; i <= stacks; ++i)
        {
            float stackAngle = glm::pi<float>() / 2 - i * stackStep;
            float xy = radius * cosf(stackAngle);
            float z = radius * sinf(stackAngle);

            for (int j = 0; j <= sectors; ++j)
            {
                float sectorAngle = j * sectorStep;
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                raw.vertices.push_back(x); raw.vertices.push_back(y); raw.vertices.push_back(z);
                raw.normals.push_back(x/radius); raw.normals.push_back(y/radius); raw.normals.push_back(z/radius);
                raw.texcoords.push_back((float)j / sectors); raw.texcoords.push_back((float)i / stacks);
            }
        }
        for (int i = 0; i < stacks; ++i)
        {
            int k1 = i * (sectors + 1);
            int k2 = k1 + sectors + 1;
            for (int j = 0; j < sectors; ++j, ++k1, ++k2)
            {
                if (i != 0) { raw.indices.push_back(k1); raw.indices.push_back(k2); raw.indices.push_back(k1 + 1); }
                if (i != (stacks - 1)) { raw.indices.push_back(k1 + 1); raw.indices.push_back(k2); raw.indices.push_back(k2 + 1); }
            }
        }
    }

    if (!raw.vertices.empty())
    {
        Mesh mesh;
        mesh.VertexCount = (uint32_t)raw.vertices.size() / 3;
        mesh.TriangleCount = (uint32_t)raw.indices.size() / 3;
        // Pre-calculating bounds
        mesh.MinBounds = { -10, -10, -10 }; mesh.MaxBounds = { 10, 10, 10 }; // Simplified
        
        // Build actual model for GPU upload (if needed immediately)
        // Usually ImportMesh will handle this, but for procedural we directly return the model
        // We'll need to upload this manually or wait for the renderer to handle it.
        model.Meshes.push_back(mesh);
        model.Materials.push_back(Material());
    }

    return model;
}
} // namespace CHEngine
