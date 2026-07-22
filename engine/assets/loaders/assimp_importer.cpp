#include "engine/assets/loaders/assimp_importer.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/common/thread_pool.h"
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "engine/core/service_locator.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <unordered_map>
#include <stb_image.h>

namespace Chained
{

static bool DecodeEmbeddedTexture(const aiTexture* texture, EmbeddedTextureData& out)
{
    if (!texture)
    {
        return false;
    }

    if (texture->mHeight == 0)
    {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(texture->pcData);
        int byteCount = (int)texture->mWidth;
        if (stbi_is_hdr_from_memory(bytes, byteCount))
        {
            CH_CORE_WARN("Assimp embedded HDR texture is not supported by the current texture pipeline. Skipping.");
            return false;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* decoded = stbi_load_from_memory(bytes, byteCount, &width, &height, &channels, 4);
        if (!decoded)
        {
            return false;
        }

        out.width = width;
        out.height = height;
        out.channels = 4;
        out.isHDR = false;
        out.data.resize((size_t)width * (size_t)height * 4);
        std::memcpy(out.data.data(), decoded, out.data.size());
        stbi_image_free(decoded);
        return true;
    }

    out.width = (int)texture->mWidth;
    out.height = (int)texture->mHeight;
    out.channels = 4;
    out.isHDR = false;
    out.data.resize((size_t)out.width * (size_t)out.height * 4);
    std::memcpy(out.data.data(), texture->pcData, out.data.size());
    return true;
}

// --- Minimalist Conversion Helpers ---
static glm::mat4 ToMat4(const aiMatrix4x4& m)
{
    return glm::transpose(glm::make_mat4(&m.a1));
}

static glm::vec3 ToVec3(const aiVector3D& v)
{
    return {v.x, v.y, v.z};
}

static glm::quat ToQuat(const aiQuaternion& q)
{
    return {q.w, q.x, q.y, q.z};
}

static glm::vec4 ToColor(const aiColor4D& c)
{
    return {c.r, c.g, c.b, c.a};
}

template <typename KeyT, typename T, typename ConvertFn, typename InterpolateFn>
static T InterpolateKeys(double time, const KeyT* keys, unsigned int count, unsigned int& lastKey,
                         const T& defaultVal, ConvertFn convert, InterpolateFn lerpFn)
{
    if (count == 0) return defaultVal;
    if (count == 1) return convert(keys[0].mValue);

    unsigned int p1 = lastKey, p2 = lastKey;
    for (unsigned int k = lastKey; k < count - 1; ++k)
    {
        if (time < keys[k + 1].mTime)
        {
            p1 = k;
            p2 = k + 1;
            lastKey = k;
            break;
        }
        p1 = k;
        p2 = k + 1;
    }

    if (time >= keys[count - 1].mTime)
        return convert(keys[count - 1].mValue);

    double dt = keys[p2].mTime - keys[p1].mTime;
    float factor = (dt > 0.0) ? (float)((time - keys[p1].mTime) / dt) : 0.0f;
    return lerpFn(convert(keys[p1].mValue), convert(keys[p2].mValue), factor);
}

static glm::vec3 InterpolatePosition(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                     const glm::vec3& defaultVal)
{
    return InterpolateKeys(time, channel->mPositionKeys, channel->mNumPositionKeys, lastKey, defaultVal,
        ToVec3, [](const glm::vec3& a, const glm::vec3& b, float f) { return glm::mix(a, b, f); });
}

static glm::quat InterpolateRotation(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                     const glm::quat& defaultVal)
{
    return InterpolateKeys(time, channel->mRotationKeys, channel->mNumRotationKeys, lastKey, defaultVal,
        ToQuat, [](const glm::quat& a, const glm::quat& b, float f) {
            aiQuaternion result;
            aiQuaternion::Interpolate(result,
                aiQuaternion(a.w, a.x, a.y, a.z),
                aiQuaternion(b.w, b.x, b.y, b.z), f);
            return ToQuat(result);
        });
}

static glm::vec3 InterpolateScale(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                  const glm::vec3& defaultVal)
{
    return InterpolateKeys(time, channel->mScalingKeys, channel->mNumScalingKeys, lastKey, defaultVal,
        ToVec3, [](const glm::vec3& a, const glm::vec3& b, float f) { return glm::mix(a, b, f); });
}

PendingModelData AssimpImporter::Import(const std::filesystem::path& path, int samplingFPS)
{
    CH_PROFILE_FUNCTION();
    PendingModelData data{};

    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });

    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights |
                         aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_CalcTangentSpace |
                         aiProcess_ImproveCacheLocality | aiProcess_OptimizeMeshes;

    if (ext != ".gltf" && ext != ".glb")
        flags |= aiProcess_FlipUVs;

    const aiScene* scene = nullptr;

    auto tryLoad = [&](const char* desc, auto loadFn) -> bool {
        try
        {
            scene = loadFn();
            return scene && scene->mRootNode;
        }
        catch (const std::exception& e)
        {
            CH_CORE_WARN("AssimpImporter: {} threw for '{}': {}", desc, path.string(), e.what());
            return false;
        }
        catch (...)
        {
            CH_CORE_WARN("AssimpImporter: {} threw for '{}' with unknown exception", desc, path.string());
            return false;
        }
    };

    tryLoad("ReadFile", [&]() { return importer.ReadFile(path.string(), flags); });

    if (!scene || !scene->mRootNode)
    {
        CH_CORE_WARN("AssimpImporter: ReadFile failed for '{}', trying memory fallback...", path.string());
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open())
        {
            std::streamsize size = file.tellg();
            if (size > 0)
            {
                file.seekg(0);
                std::vector<char> buffer(static_cast<size_t>(size), 0);
                file.read(buffer.data(), size);
                tryLoad("ReadFileFromMemory", [&]() {
                    return importer.ReadFileFromMemory(buffer.data(), static_cast<size_t>(size), flags,
                                                      path.extension().string().c_str());
                });
            }
        }
    }

    if (!scene || !scene->mRootNode)
    {
        CH_CORE_ERROR("Assimp Model Load Failed: {} | Error: {}", path.filename().string(),
                      importer.GetErrorString());
        return data;
    }

    AssimpImporter instance(path, samplingFPS, scene);
    return instance.Execute();
}

AssimpImporter::AssimpImporter(const std::filesystem::path& path, int samplingFPS, const aiScene* scene)
    : m_Path(path),
      m_SamplingFPS(samplingFPS),
      m_Scene(scene)
{
    m_ModelDir = path.parent_path();
}

PendingModelData AssimpImporter::Execute()
{
    ProcessHierarchy();
    ProcessMeshes();
    ProcessMaterials();
    DecodeEmbeddedTextures();
    ProcessAnimations();
    MergeMeshesByMaterial();

    m_Data.isValid = true;
    return std::move(m_Data);
}

void AssimpImporter::ProcessNode(aiNode* node, int parentIdx)
{
    if (!node)
    {
        return;
    }

    int currentIdx = (int)m_Data.nodeNames.size();
    m_Data.nodeNames.push_back(node->mName.C_Str());
    m_Data.nodeParents.push_back(parentIdx);

    glm::mat4 local = ToMat4(node->mTransformation);
    m_Data.nodeLocalTransforms.push_back(local);
    m_Data.globalBindPoses.push_back((parentIdx == -1) ? local : m_Data.globalBindPoses[parentIdx] * local);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex < m_Scene->mNumMeshes)
        {
            m_Data.instances.push_back({(int)meshIndex, m_Data.globalBindPoses[currentIdx]});
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], currentIdx);
    }
}

void AssimpImporter::ProcessHierarchy()
{
    CH_PROFILE_SCOPE("AssimpImporter::ProcessHierarchy");
    ProcessNode(m_Scene->mRootNode, -1);

    m_Data.nodeCount = (int)m_Data.nodeNames.size();
    m_NameToIndex.reserve(m_Data.nodeNames.size());
    for (int i = 0; i < (int)m_Data.nodeNames.size(); ++i)
    {
        m_NameToIndex[m_Data.nodeNames[i]] = i;
    }
}

void AssimpImporter::ProcessSingleMesh(uint32_t m)
{
    aiMesh* am = m_Scene->mMeshes[m];
    RawMesh rm;
    rm.materialIndex = am->mMaterialIndex;

    rm.vertices.reserve((size_t)am->mNumVertices * 3);
    if (am->mTextureCoords[0])
    {
        rm.texcoords.reserve((size_t)am->mNumVertices * 2);
    }
    if (am->mNormals)
    {
        rm.normals.reserve((size_t)am->mNumVertices * 3);
    }
    if (am->mTangents)
    {
        rm.tangents.reserve((size_t)am->mNumVertices * 3);
    }
    if (am->mColors[0])
    {
        rm.colors.reserve((size_t)am->mNumVertices * 4);
    }
    rm.indices.reserve((size_t)am->mNumFaces * 3);

    for (unsigned int v = 0; v < am->mNumVertices; ++v)
    {
        rm.vertices.insert(rm.vertices.end(), {am->mVertices[v].x, am->mVertices[v].y, am->mVertices[v].z});
        if (am->mTextureCoords[0])
        {
            rm.texcoords.insert(rm.texcoords.end(), {am->mTextureCoords[0][v].x, am->mTextureCoords[0][v].y});
        }
        if (am->mNormals)
        {
            rm.normals.insert(rm.normals.end(), {am->mNormals[v].x, am->mNormals[v].y, am->mNormals[v].z});
        }
        if (am->mTangents)
        {
            rm.tangents.insert(rm.tangents.end(), {am->mTangents[v].x, am->mTangents[v].y, am->mTangents[v].z});
        }
        if (am->mColors[0])
        {
            rm.colors.insert(rm.colors.end(), {
                (unsigned char)(am->mColors[0][v].r * 255.0f),
                (unsigned char)(am->mColors[0][v].g * 255.0f),
                (unsigned char)(am->mColors[0][v].b * 255.0f),
                (unsigned char)(am->mColors[0][v].a * 255.0f)
            });
        }
    }

    for (unsigned int f = 0; f < am->mNumFaces; ++f)
    {
        const aiFace& face = am->mFaces[f];
        if (face.mNumIndices != 3)
        {
            continue;
        }
        rm.indices.insert(rm.indices.end(), {face.mIndices[0], face.mIndices[1], face.mIndices[2]});
    }

    rm.MinBounds = {FLT_MAX, FLT_MAX, FLT_MAX};
    rm.MaxBounds = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    for (size_t v = 0; v < rm.vertices.size(); v += 3)
    {
        glm::vec3 pos = {rm.vertices[v], rm.vertices[v + 1], rm.vertices[v + 2]};
        rm.MinBounds = glm::min(rm.MinBounds, pos);
        rm.MaxBounds = glm::max(rm.MaxBounds, pos);
    }

    if (am->mNumBones > 0)
    {
        rm.joints.resize((size_t)am->mNumVertices * 4, 0);
        rm.weights.resize((size_t)am->mNumVertices * 4, 0.0f);
        std::vector<int> jointCounts(am->mNumVertices, 0);
        auto& offsetWrites = m_MeshOffsetWrites[m];
        offsetWrites.reserve(am->mNumBones);

        for (unsigned int b = 0; b < am->mNumBones; ++b)
        {
            aiBone* bone = am->mBones[b];
            auto boneIt = m_NameToIndex.find(bone->mName.C_Str());
            if (boneIt == m_NameToIndex.end())
            {
                continue;
            }

            const int boneIdx = boneIt->second;
            offsetWrites.emplace_back(boneIdx, ToMat4(bone->mOffsetMatrix));

            for (unsigned int w = 0; w < bone->mNumWeights; ++w)
            {
                const int vIdx = bone->mWeights[w].mVertexId;
                if (vIdx < 0 || vIdx >= (int)am->mNumVertices)
                {
                    continue;
                }

                if (jointCounts[vIdx] < 4)
                {
                    const int slot = vIdx * 4 + jointCounts[vIdx]++;
                    rm.joints[slot] = (unsigned char)boneIdx;
                    rm.weights[slot] = bone->mWeights[w].mWeight;
                }
            }
        }
    }
    m_Data.meshes[m] = std::move(rm);
}

void AssimpImporter::ProcessMeshes()
{
    CH_PROFILE_SCOPE("AssimpImporter::ProcessMeshes");
    m_Data.meshes.resize(m_Scene->mNumMeshes);
    m_MeshOffsetWrites.resize(m_Scene->mNumMeshes);

    if (m_Scene->mNumMeshes == 0)
    {
        return;
    }
    else if (m_Scene->mNumMeshes == 1)
    {
        ProcessSingleMesh(0);
    }
    else
    {
        std::vector<std::future<void>> futures;
        futures.reserve(m_Scene->mNumMeshes);
        for (uint32_t m = 0; m < m_Scene->mNumMeshes; ++m)
        {
            futures.push_back(ServiceLocator::Get<ThreadPool>()->Enqueue([this, m]() { ProcessSingleMesh(m); }));
        }
        for (auto& ft : futures)
        {
            ft.wait();
        }
    }

    m_Data.offsetMatrices.assign(m_Data.nodeNames.size(), glm::mat4(1.0f));
    for (const auto& meshOffsets : m_MeshOffsetWrites)
    {
        for (const auto& [boneIdx, offset] : meshOffsets)
        {
            if (boneIdx >= 0 && boneIdx < (int)m_Data.offsetMatrices.size())
            {
                m_Data.offsetMatrices[boneIdx] = offset;
            }
        }
    }
}

void AssimpImporter::ProcessMaterials()
{
    CH_PROFILE_SCOPE("AssimpImporter::ProcessMaterials");
    m_Data.materials.resize(m_Scene->mNumMaterials);
    for (unsigned int i = 0; i < m_Scene->mNumMaterials; ++i)
    {
        aiMaterial* am = m_Scene->mMaterials[i];
        RawMaterial& rm = m_Data.materials[i];

        aiString matName;
        if (am->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS)
        {
            rm.name = matName.C_Str();
        }

        aiColor4D col(1.0f, 1.0f, 1.0f, 1.0f);
        if (aiGetMaterialColor(am, AI_MATKEY_BASE_COLOR, &col) == AI_SUCCESS)
        {
            rm.albedoColor = ToColor(col);
        }
        else if (aiGetMaterialColor(am, AI_MATKEY_COLOR_DIFFUSE, &col) == AI_SUCCESS)
        {
            rm.albedoColor = ToColor(col);
        }

        if (rm.albedoColor.a < 0.001f)
        {
            rm.albedoColor.a = 1.0f;
        }

        float opacity = 1.0f;
        if (aiGetMaterialFloat(am, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS)
        {
            rm.albedoColor.a *= opacity;
        }

        if (aiGetMaterialColor(am, AI_MATKEY_COLOR_EMISSIVE, &col) == AI_SUCCESS)
        {
            rm.emissiveColor = ToColor(col);
        }

        aiGetMaterialFloat(am, AI_MATKEY_EMISSIVE_INTENSITY, &rm.emissiveIntensity);
        aiGetMaterialFloat(am, AI_MATKEY_METALLIC_FACTOR, &rm.metalness);
        aiGetMaterialFloat(am, AI_MATKEY_ROUGHNESS_FACTOR, &rm.roughness);

        auto resolvePath = [&](const std::string& texPath) -> std::string {
            if (texPath.empty())
            {
                return "";
            }
            if (std::filesystem::exists(texPath))
            {
                return texPath;
            }

            std::filesystem::path p1 = m_ModelDir / texPath;
            if (std::filesystem::exists(p1))
            {
                return p1.string();
            }

            std::string filename = std::filesystem::path(texPath).filename().string();
            std::filesystem::path p2 = m_ModelDir / filename;
            if (std::filesystem::exists(p2))
            {
                return p2.string();
            }

            std::filesystem::path p3 = m_ModelDir / "textures" / filename;
            if (std::filesystem::exists(p3))
            {
                return p3.string();
            }

            return texPath;
        };

        auto getTex = [&](aiTextureType type) -> std::string {
            aiString str;
            if (am->GetTexture(type, 0, &str) == AI_SUCCESS)
            {
                return resolvePath(str.C_Str());
            }
            return "";
        };

        auto getTexWithFallback = [&](aiTextureType primary, aiTextureType fallback) -> std::string {
            std::string path = getTex(primary);
            return path.empty() ? getTex(fallback) : path;
        };

        rm.albedoPath = getTexWithFallback(aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR);
        rm.normalPath = getTexWithFallback(aiTextureType_NORMALS, aiTextureType_HEIGHT);
        rm.emissivePath = getTex(aiTextureType_EMISSIVE);
        rm.metallicRoughnessPath = getTexWithFallback(aiTextureType_METALNESS, aiTextureType_UNKNOWN);
        rm.occlusionPath = getTexWithFallback(aiTextureType_LIGHTMAP, aiTextureType_AMBIENT_OCCLUSION);

        int blendMode = 0;
        if (aiGetMaterialInteger(am, AI_MATKEY_BLEND_FUNC, &blendMode) == AI_SUCCESS)
        {
            if (blendMode != aiBlendMode_Default && blendMode != aiBlendMode_Additive)
            {
                rm.transparent = true;
            }
        }

        if (rm.albedoColor.a < 0.999f || opacity < 0.999f)
        {
            rm.transparent = true;
        }

        unsigned int isTransparent = 0;
        if (aiGetMaterialInteger(am, "$mat.isTransparent", 0, 0, (int*)&isTransparent) == AI_SUCCESS)
        {
            if (isTransparent)
            {
                rm.transparent = true;
            }
        }
    }
}

void AssimpImporter::DecodeEmbeddedTextures()
{
    CH_PROFILE_SCOPE("AssimpImporter::DecodeEmbeddedTextures");
    for (unsigned int i = 0; i < m_Scene->mNumTextures; ++i)
    {
        const aiTexture* texture = m_Scene->mTextures[i];
        if (!texture)
        {
            continue;
        }

        EmbeddedTextureData embedded;
        if (!DecodeEmbeddedTexture(texture, embedded))
        {
            continue;
        }

        m_Data.embeddedTextures.emplace("*" + std::to_string(i), std::move(embedded));
    }
}

void AssimpImporter::ProcessAnimations()
{
    CH_PROFILE_SCOPE("AssimpImporter::ProcessAnimations");
    m_Data.animations.resize(m_Scene->mNumAnimations);
    for (unsigned int a = 0; a < m_Scene->mNumAnimations; ++a)
    {
        aiAnimation* anim = m_Scene->mAnimations[a];
        RawAnimation& ra = m_Data.animations[a];
        ra.name = anim->mName.C_Str();
        if (ra.name.empty())
        {
            ra.name = "Anim_" + std::to_string(a);
        }

        const double ticksPerSecond =
            (anim->mTicksPerSecond > 0.0) ? anim->mTicksPerSecond : (double)std::max(1, m_SamplingFPS);
        ra.frameRate = (float)std::max(1, m_SamplingFPS);

        double durationTicks = anim->mDuration;
        if (durationTicks == 0.0)
        {
            for (unsigned int c = 0; c < anim->mNumChannels; ++c)
            {
                if (anim->mChannels[c]->mNumPositionKeys > 0)
                {
                    durationTicks =
                        std::max(durationTicks,
                                 anim->mChannels[c]->mPositionKeys[anim->mChannels[c]->mNumPositionKeys - 1].mTime);
                }
                if (anim->mChannels[c]->mNumRotationKeys > 0)
                {
                    durationTicks =
                        std::max(durationTicks,
                                 anim->mChannels[c]->mRotationKeys[anim->mChannels[c]->mNumRotationKeys - 1].mTime);
                }
                if (anim->mChannels[c]->mNumScalingKeys > 0)
                {
                    durationTicks = std::max(
                        durationTicks, anim->mChannels[c]->mScalingKeys[anim->mChannels[c]->mNumScalingKeys - 1].mTime);
                }
            }
        }

        const double durationSeconds = (ticksPerSecond > 0.0) ? (durationTicks / ticksPerSecond) : 0.0;
        ra.frameCount = std::max(1, (int)std::ceil(durationSeconds * (double)ra.frameRate) + 1);
        ra.boneCount = (int)m_Data.nodeNames.size();
        ra.framePoses.resize(ra.frameCount * ra.boneCount);
        const double ticksPerFrame = ticksPerSecond / (double)ra.frameRate;

        std::vector<TransformData> bindPoses(ra.boneCount);
        for (int b = 0; b < ra.boneCount; ++b)
        {
            aiNode* node = m_Scene->mRootNode->FindNode(m_Data.nodeNames[b].c_str());
            if (node)
            {
                aiVector3D p, s;
                aiQuaternion r;
                node->mTransformation.Decompose(s, r, p);
                bindPoses[b] = {ToVec3(p), ToQuat(r), ToVec3(s)};
            }
            else
            {
                bindPoses[b] = {glm::vec3(0), glm::quat(1, 0, 0, 0), glm::vec3(1)};
            }
        }

        for (int f = 0; f < ra.frameCount; ++f)
        {
            for (int b = 0; b < ra.boneCount; ++b)
            {
                ra.framePoses[f * ra.boneCount + b] = bindPoses[b];
            }
        }

        std::vector<std::future<void>> animFutures;
        animFutures.reserve(anim->mNumChannels);
        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
        {
            aiNodeAnim* channel = anim->mChannels[c];
            auto boneIt = m_NameToIndex.find(channel->mNodeName.C_Str());
            if (boneIt == m_NameToIndex.end())
            {
                continue;
            }

            const int boneIdx = boneIt->second;
            animFutures.push_back(ServiceLocator::Get<ThreadPool>()->Enqueue([&ra, channel, boneIdx, &bindPoses, ticksPerFrame]() {
                unsigned int lastPosKey = 0, lastRotKey = 0, lastSclKey = 0;
                for (int f = 0; f < ra.frameCount; ++f)
                {
                    double time = (double)f * ticksPerFrame;
                    glm::vec3 pos = InterpolatePosition(time, channel, lastPosKey, bindPoses[boneIdx].translation);
                    glm::quat rot = InterpolateRotation(time, channel, lastRotKey, bindPoses[boneIdx].rotation);
                    glm::vec3 scale = InterpolateScale(time, channel, lastSclKey, bindPoses[boneIdx].scale);
                    ra.framePoses[f * ra.boneCount + boneIdx] = {pos, rot, scale};
                }
            }));
        }
        for (auto& f : animFutures) f.get();
        CH_CORE_INFO("AssimpImporter: Loaded animation '{}' ({} frames, {} fps, {} channels)", ra.name, ra.frameCount, ra.frameRate, anim->mNumChannels);
    }
}

void AssimpImporter::MergeMeshesByMaterial()
{
    CH_PROFILE_FUNCTION();
    if (m_Data.instances.empty() || m_Data.meshes.empty())
    {
        return;
    }

    struct InstanceGroup
    {
        std::vector<int> instanceIndices;
    };
    std::unordered_map<int, InstanceGroup> groups;
    for (int i = 0; i < (int)m_Data.instances.size(); ++i)
    {
        int meshIdx = m_Data.instances[i].meshIndex;
        if (meshIdx >= 0 && meshIdx < (int)m_Data.meshes.size())
        {
            groups[m_Data.meshes[meshIdx].materialIndex].instanceIndices.push_back(i);
        }
    }

    CH_CORE_INFO("AssimpImporter: Merging {0} instances into {1} material groups", m_Data.instances.size(),
                 groups.size());

    std::vector<RawMesh> mergedMeshes;
    std::vector<MeshInstance> mergedInstances;
    mergedMeshes.reserve(groups.size());
    mergedInstances.reserve(groups.size());

    for (auto& [matIdx, group] : groups)
    {
        RawMesh merged;
        merged.materialIndex = matIdx;

        size_t totalVertices = 0;
        size_t totalIndices = 0;
        for (int instIdx : group.instanceIndices)
        {
            int srcIdx = m_Data.instances[instIdx].meshIndex;
            totalVertices += m_Data.meshes[srcIdx].vertices.size();
            totalIndices += m_Data.meshes[srcIdx].indices.size();
        }

        merged.vertices.reserve(totalVertices);
        merged.texcoords.reserve(totalVertices / 3 * 2);
        merged.normals.reserve(totalVertices);
        merged.tangents.reserve(totalVertices);
        merged.colors.reserve(totalVertices / 3 * 4);
        merged.indices.reserve(totalIndices);

        int firstMeshIdx = m_Data.instances[group.instanceIndices[0]].meshIndex;
        bool hasSkins = !m_Data.meshes[firstMeshIdx].joints.empty();
        if (hasSkins)
        {
            merged.joints.reserve(totalVertices / 3 * 4);
            merged.weights.reserve(totalVertices / 3 * 4);
        }

        merged.MinBounds = {FLT_MAX, FLT_MAX, FLT_MAX};
        merged.MaxBounds = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for (int instIdx : group.instanceIndices)
        {
            const auto& inst = m_Data.instances[instIdx];
            const auto& src = m_Data.meshes[inst.meshIndex];
            uint32_t vertexOffset = (uint32_t)(merged.vertices.size() / 3);

            glm::mat4 t = inst.localTransform;
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(t)));

            for (size_t v = 0; v < src.vertices.size(); v += 3)
            {
                if (!hasSkins)
                {
                    glm::vec4 pos = t * glm::vec4(src.vertices[v], src.vertices[v + 1], src.vertices[v + 2], 1.0f);
                    merged.vertices.insert(merged.vertices.end(), {pos.x, pos.y, pos.z});
                }
                else
                {
                    merged.vertices.insert(merged.vertices.end(),
                                           {src.vertices[v], src.vertices[v + 1], src.vertices[v + 2]});
                }
            }

            merged.texcoords.insert(merged.texcoords.end(), src.texcoords.begin(), src.texcoords.end());
            merged.colors.insert(merged.colors.end(), src.colors.begin(), src.colors.end());

            for (size_t n = 0; n < src.normals.size(); n += 3)
            {
                if (!hasSkins)
                {
                    glm::vec3 norm = glm::normalize(normalMatrix *
                                                    glm::vec3(src.normals[n], src.normals[n + 1], src.normals[n + 2]));
                    merged.normals.insert(merged.normals.end(), {norm.x, norm.y, norm.z});
                }
                else
                {
                    merged.normals.insert(merged.normals.end(),
                                          {src.normals[n], src.normals[n + 1], src.normals[n + 2]});
                }
            }

            for (size_t t = 0; t < src.tangents.size(); t += 3)
            {
                if (!hasSkins)
                {
                    glm::vec3 tan = glm::normalize(normalMatrix *
                                                   glm::vec3(src.tangents[t], src.tangents[t + 1], src.tangents[t + 2]));
                    merged.tangents.insert(merged.tangents.end(), {tan.x, tan.y, tan.z});
                }
                else
                {
                    merged.tangents.insert(merged.tangents.end(),
                                           {src.tangents[t], src.tangents[t + 1], src.tangents[t + 2]});
                }
            }

            for (uint32_t idx : src.indices)
            {
                merged.indices.push_back(idx + vertexOffset);
            }

            if (hasSkins)
            {
                if (!src.joints.empty())
                {
                    merged.joints.insert(merged.joints.end(), src.joints.begin(), src.joints.end());
                    merged.weights.insert(merged.weights.end(), src.weights.begin(), src.weights.end());
                }
                else
                {
                    size_t padSize = (src.vertices.size() / 3) * 4;
                    merged.joints.insert(merged.joints.end(), padSize, (unsigned char)0);
                    merged.weights.insert(merged.weights.end(), padSize, 0.0f);
                }
            }

            if (!hasSkins)
            {
                glm::vec3 corners[8] = {
                    {src.MinBounds.x, src.MinBounds.y, src.MinBounds.z},
                    {src.MaxBounds.x, src.MinBounds.y, src.MinBounds.z},
                    {src.MinBounds.x, src.MaxBounds.y, src.MinBounds.z},
                    {src.MaxBounds.x, src.MaxBounds.y, src.MinBounds.z},
                    {src.MinBounds.x, src.MinBounds.y, src.MaxBounds.z},
                    {src.MaxBounds.x, src.MinBounds.y, src.MaxBounds.z},
                    {src.MinBounds.x, src.MaxBounds.y, src.MaxBounds.z},
                    {src.MaxBounds.x, src.MaxBounds.y, src.MaxBounds.z}
                };
                for (auto& c : corners)
                {
                    glm::vec3 tp = glm::vec3(t * glm::vec4(c, 1.0f));
                    merged.MinBounds = glm::min(merged.MinBounds, tp);
                    merged.MaxBounds = glm::max(merged.MaxBounds, tp);
                }
            }
            else
            {
                for (size_t v = 0; v < src.vertices.size(); v += 3)
                {
                    glm::vec3 p = {src.vertices[v], src.vertices[v + 1], src.vertices[v + 2]};
                    merged.MinBounds = glm::min(merged.MinBounds, p);
                    merged.MaxBounds = glm::max(merged.MaxBounds, p);
                }
            }
        }

        int newMeshIdx = (int)mergedMeshes.size();
        mergedMeshes.push_back(std::move(merged));
        mergedInstances.push_back({newMeshIdx, glm::mat4(1.0f)});
    }

    m_Data.meshes = std::move(mergedMeshes);
    m_Data.instances = std::move(mergedInstances);
}
} // namespace Chained