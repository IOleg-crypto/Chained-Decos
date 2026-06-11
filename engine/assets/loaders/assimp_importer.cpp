#include "engine/graphics/loaders/assimp_importer.h"
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/core/thread_pool.h"
#include <future>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb_image.h>
#include <string>
#include <mutex>

namespace CHEngine
{
static bool IsSupportedAssimpExtension(const std::string& ext)
{
    return ext == ".gltf" || ext == ".glb" || ext == ".obj";
}

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

static glm::vec3 InterpolatePosition(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                     const glm::vec3& defaultVal)
{
    if (channel->mNumPositionKeys == 0) return defaultVal;
    if (channel->mNumPositionKeys == 1) return ToVec3(channel->mPositionKeys[0].mValue);

    unsigned int p1 = lastKey, p2 = lastKey;
    for (unsigned int k = lastKey; k < channel->mNumPositionKeys - 1; ++k)
    {
        if (time < channel->mPositionKeys[k + 1].mTime)
        {
            p1 = k; p2 = k + 1; lastKey = k;
            break;
        }
        p1 = k; p2 = k + 1;
    }

    if (time >= channel->mPositionKeys[channel->mNumPositionKeys - 1].mTime)
        return ToVec3(channel->mPositionKeys[channel->mNumPositionKeys - 1].mValue);

    double dt = channel->mPositionKeys[p2].mTime - channel->mPositionKeys[p1].mTime;
    float factor = (dt > 0.0) ? (float)((time - channel->mPositionKeys[p1].mTime) / dt) : 0.0f;
    return glm::mix(ToVec3(channel->mPositionKeys[p1].mValue), ToVec3(channel->mPositionKeys[p2].mValue), factor);
}

static glm::quat InterpolateRotation(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                     const glm::quat& defaultVal)
{
    if (channel->mNumRotationKeys == 0) return defaultVal;
    if (channel->mNumRotationKeys == 1) return ToQuat(channel->mRotationKeys[0].mValue);

    unsigned int p1 = lastKey, p2 = lastKey;
    for (unsigned int k = lastKey; k < channel->mNumRotationKeys - 1; ++k)
    {
        if (time < channel->mRotationKeys[k + 1].mTime)
        {
            p1 = k; p2 = k + 1; lastKey = k;
            break;
        }
        p1 = k; p2 = k + 1;
    }

    if (time >= channel->mRotationKeys[channel->mNumRotationKeys - 1].mTime)
        return ToQuat(channel->mRotationKeys[channel->mNumRotationKeys - 1].mValue);

    double dt = channel->mRotationKeys[p2].mTime - channel->mRotationKeys[p1].mTime;
    float factor = (dt > 0.0) ? (float)((time - channel->mRotationKeys[p1].mTime) / dt) : 0.0f;
    aiQuaternion interpolated;
    aiQuaternion::Interpolate(interpolated, channel->mRotationKeys[p1].mValue, channel->mRotationKeys[p2].mValue, factor);
    return ToQuat(interpolated);
}

static glm::vec3 InterpolateScale(double time, const aiNodeAnim* channel, unsigned int& lastKey,
                                   const glm::vec3& defaultVal)
{
    if (channel->mNumScalingKeys == 0) return defaultVal;
    if (channel->mNumScalingKeys == 1) return ToVec3(channel->mScalingKeys[0].mValue);

    unsigned int p1 = lastKey, p2 = lastKey;
    for (unsigned int k = lastKey; k < channel->mNumScalingKeys - 1; ++k)
    {
        if (time < channel->mScalingKeys[k + 1].mTime)
        {
            p1 = k; p2 = k + 1; lastKey = k;
            break;
        }
        p1 = k; p2 = k + 1;
    }

    if (time >= channel->mScalingKeys[channel->mNumScalingKeys - 1].mTime)
        return ToVec3(channel->mScalingKeys[channel->mNumScalingKeys - 1].mValue);

    double dt = channel->mScalingKeys[p2].mTime - channel->mScalingKeys[p1].mTime;
    float factor = (dt > 0.0) ? (float)((time - channel->mScalingKeys[p1].mTime) / dt) : 0.0f;
    return glm::mix(ToVec3(channel->mScalingKeys[p1].mValue), ToVec3(channel->mScalingKeys[p2].mValue), factor);
}

PendingModelData AssimpImporter::Import(const std::filesystem::path& path, int samplingFPS, ThreadPool* threadPool)
{
    CH_PROFILE_FUNCTION();
    PendingModelData data;
    try
    {
        Assimp::Importer importer;
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
        importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (!IsSupportedAssimpExtension(ext)) return data;

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights |
                             aiProcess_JoinIdenticalVertices | aiProcess_SortByPType | aiProcess_CalcTangentSpace |
                             aiProcess_ImproveCacheLocality | aiProcess_OptimizeMeshes;

        if (ext != ".gltf" && ext != ".glb") flags |= aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(path.string(), flags);
        if (!scene || !scene->mRootNode)
        {
            CH_CORE_ERROR("Assimp Model Load Failed: {0}", path.string());
            return data;
        }

        AssimpImporter instance(path, samplingFPS, scene, threadPool);
        data = instance.Execute();
        return data;
    }
    catch (const std::exception& e)
    {
        CH_CORE_ERROR("AssimpImporter: Exception importing '{0}': {1}", path.string(), e.what());
        return data;
    }
}

AssimpImporter::AssimpImporter(const std::filesystem::path& path, int samplingFPS, const aiScene* scene, ThreadPool* threadPool)
    : m_Path(path), m_SamplingFPS(samplingFPS), m_Scene(scene), m_ThreadPool(threadPool)
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

    if (m_Data.instances.size() < 100)
    {
        MergeMeshesByMaterial();
    }
    else
    {
        CH_CORE_INFO("AssimpImporter: Skipping MergeMeshesByMaterial for complex model ({0} instances).", m_Data.instances.size());
    }

    m_Data.isValid = true;
    return std::move(m_Data);
}

void AssimpImporter::ProcessNode(aiNode* node, int parentIdx)
{
    if (!node) return;
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
    for (int i = 0; i < (int)m_Data.nodeNames.size(); ++i)
    {
        m_NameToIndex.emplace(std::string(m_Data.nodeNames[i].c_str()), i);
    }
}

void AssimpImporter::ProcessSingleMesh(uint32_t m)
{
    aiMesh* am = m_Scene->mMeshes[m];
    auto& rm = m_Data.meshes[m];
    rm.materialIndex = am->mMaterialIndex;

    rm.vertices.reserve((size_t)am->mNumVertices * 3);
    if (am->mTextureCoords[0]) rm.texcoords.reserve((size_t)am->mNumVertices * 2);
    if (am->mNormals) rm.normals.reserve((size_t)am->mNumVertices * 3);
    if (am->mTangents) rm.tangents.reserve((size_t)am->mNumVertices * 3);
    rm.indices.reserve((size_t)am->mNumFaces * 3);

    for (unsigned int v = 0; v < am->mNumVertices; ++v)
    {
        rm.vertices.insert(rm.vertices.end(), {am->mVertices[v].x, am->mVertices[v].y, am->mVertices[v].z});
        if (am->mTextureCoords[0])
            rm.texcoords.insert(rm.texcoords.end(), {am->mTextureCoords[0][v].x, am->mTextureCoords[0][v].y});
        if (am->mNormals)
            rm.normals.insert(rm.normals.end(), {am->mNormals[v].x, am->mNormals[v].y, am->mNormals[v].z});
        if (am->mTangents)
            rm.tangents.insert(rm.tangents.end(), {am->mTangents[v].x, am->mTangents[v].y, am->mTangents[v].z});
    }

    for (unsigned int f = 0; f < am->mNumFaces; ++f)
    {
        const aiFace& face = am->mFaces[f];
        if (face.mNumIndices != 3) continue;
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

        for (unsigned int b = 0; b < am->mNumBones; ++b)
        {
            aiBone* bone = am->mBones[b];
            auto boneIt = m_NameToIndex.find(std::string(bone->mName.C_Str()));
            if (boneIt == m_NameToIndex.end()) continue;

            const int boneIdx = boneIt->second;
            offsetWrites.emplace_back(boneIdx, ToMat4(bone->mOffsetMatrix));

            for (unsigned int w = 0; w < bone->mNumWeights; ++w)
            {
                const int vIdx = bone->mWeights[w].mVertexId;
                if (vIdx < 0 || vIdx >= (int)am->mNumVertices) continue;
                if (jointCounts[vIdx] < 4)
                {
                    const int slot = vIdx * 4 + jointCounts[vIdx]++;
                    rm.joints[slot] = (unsigned char)boneIdx;
                    rm.weights[slot] = bone->mWeights[w].mWeight;
                }
            }
        }
    }
}

void AssimpImporter::ProcessMeshes()
{
    CH_PROFILE_SCOPE("AssimpImporter::ProcessMeshes");
    m_Data.meshes.resize(m_Scene->mNumMeshes);
    m_MeshOffsetWrites.resize(m_Scene->mNumMeshes);

    if (m_ThreadPool && m_Scene->mNumMeshes > 1)
    {
        std::vector<std::future<void>> futures;
        futures.reserve(m_Scene->mNumMeshes);
        for (uint32_t m = 0; m < (uint32_t)m_Scene->mNumMeshes; ++m)
            futures.push_back(m_ThreadPool->Enqueue([this, m]() { ProcessSingleMesh(m); }));
        for (auto& ft : futures) ft.get();
    }
    else
    {
        for (uint32_t m = 0; m < (uint32_t)m_Scene->mNumMeshes; ++m) ProcessSingleMesh(m);
    }

    m_Data.offsetMatrices.assign((size_t)m_Data.nodeNames.size(), glm::mat4(1.0f));
    for (const auto& meshOffsets : m_MeshOffsetWrites)
        for (const auto& [boneIdx, offset] : meshOffsets)
            if (boneIdx >= 0 && boneIdx < (int)m_Data.offsetMatrices.size())
                m_Data.offsetMatrices[boneIdx] = offset;
}

void AssimpImporter::ProcessMaterials()
{
    CH_PROFILE_SCOPE("AssimpImporter::ProcessMaterials");
    m_Data.materials.resize(m_Scene->mNumMaterials);
    for (unsigned int i = 0; i < m_Scene->mNumMaterials; ++i)
    {
        aiMaterial* am = m_Scene->mMaterials[i];
        RawMaterial& rm = m_Data.materials[i];
        aiColor4D col(1.0f, 1.0f, 1.0f, 1.0f);
        if (aiGetMaterialColor(am, AI_MATKEY_BASE_COLOR, &col) == AI_SUCCESS ||
            aiGetMaterialColor(am, AI_MATKEY_COLOR_DIFFUSE, &col) == AI_SUCCESS)
            rm.albedoColor = ToColor(col);
        if (rm.albedoColor.a < 0.001f) rm.albedoColor.a = 1.0f;

        float opacity = 1.0f;
        if (aiGetMaterialFloat(am, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS)
            rm.albedoColor.a *= opacity;

        auto resolvePath = [&](const std::string& texPath) -> std::string {
            if (texPath.empty()) return "";
            if (std::filesystem::exists(texPath)) return texPath;
            std::filesystem::path p1 = m_ModelDir / texPath;
            if (std::filesystem::exists(p1)) return p1.string();
            std::string filename = std::filesystem::path(texPath).filename().string();
            std::filesystem::path p2 = m_ModelDir / filename;
            if (std::filesystem::exists(p2)) return p2.string();
            return texPath;
        };

        auto getTex = [&](aiTextureType type) -> std::string {
            aiString str;
            if (am->GetTexture(type, 0, &str) == AI_SUCCESS) return resolvePath(str.C_Str());
            return "";
        };

        rm.albedoPath = getTex(aiTextureType_DIFFUSE);
        if (rm.albedoPath.empty()) rm.albedoPath = getTex(aiTextureType_BASE_COLOR);
        rm.normalPath = getTex(aiTextureType_NORMALS);
        rm.metallicRoughnessPath = getTex(aiTextureType_METALNESS);
    }
}

void AssimpImporter::DecodeEmbeddedTextures()
{
    CH_PROFILE_SCOPE("AssimpImporter::DecodeEmbeddedTextures");
    if (!m_Scene->HasTextures()) return;
    if (m_ThreadPool && m_Scene->mNumTextures > 1)
    {
        std::pmr::memory_resource* resource = m_Data.meshes.get_allocator().resource();
        std::vector<std::future<void>> futures;
        futures.reserve(m_Scene->mNumTextures);
        std::mutex mapMutex;
        for (unsigned int i = 0; i < m_Scene->mNumTextures; ++i)
        {
            futures.push_back(m_ThreadPool->Enqueue([this, i, resource, &mapMutex] {
                aiTexture* tex = m_Scene->mTextures[i];
                EmbeddedTextureData etd(resource);
                if (DecodeEmbeddedTexture(tex, etd))
                {
                    std::string name = "*" + std::to_string(i);
                    std::lock_guard<std::mutex> lock(mapMutex);
                    m_Data.embeddedTextures.emplace(std::pmr::string(name.c_str(), resource), std::move(etd));
                }
            }));
        }
        for (auto& f : futures) f.get();
    }
    else
    {
        for (unsigned int i = 0; i < m_Scene->mNumTextures; ++i)
        {
            aiTexture* tex = m_Scene->mTextures[i];
            EmbeddedTextureData etd(m_Data.meshes.get_allocator().resource());
            if (DecodeEmbeddedTexture(tex, etd))
            {
                std::string name = "*" + std::to_string(i);
                m_Data.embeddedTextures.emplace(std::pmr::string(name.c_str(), m_Data.meshes.get_allocator().resource()), std::move(etd));
            }
        }
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
        if (ra.name.empty()) ra.name = "Anim_" + std::to_string(a);

        const double ticksPerSecond = (anim->mTicksPerSecond > 0.0) ? anim->mTicksPerSecond : (double)std::max(1, m_SamplingFPS);
        ra.frameRate = (float)std::max(1, m_SamplingFPS);
        double durationTicks = anim->mDuration;
        const double durationSeconds = (ticksPerSecond > 0.0) ? (durationTicks / ticksPerSecond) : 0.0;
        ra.frameCount = std::max(1, (int)std::ceil(durationSeconds * (double)ra.frameRate) + 1);
        ra.boneCount = (int)m_Data.nodeNames.size();
        ra.framePoses.resize(ra.frameCount * ra.boneCount);
        const double ticksPerFrame = ticksPerSecond / (double)ra.frameRate;

        struct NodeBinding { glm::vec3 translation; glm::vec3 scale; glm::quat rotation; };
        std::vector<NodeBinding> bindPoses(m_Data.nodeNames.size());
        for (int i = 0; i < (int)m_Data.nodeNames.size(); ++i)
        {
            glm::mat4 t = m_Data.nodeLocalTransforms[i];
            bindPoses[i].translation = glm::vec3(t[3]);
            bindPoses[i].scale = glm::vec3(glm::length(t[0]), glm::length(t[1]), glm::length(t[2]));
            bindPoses[i].rotation = glm::normalize(glm::quat_cast(t));
        }

        for (unsigned int c = 0; c < anim->mNumChannels; ++c)
        {
            const aiNodeAnim* channel = anim->mChannels[c];
            auto it = m_NameToIndex.find(std::string(channel->mNodeName.C_Str()));
            if (it == m_NameToIndex.end()) continue;
            const int boneIdx = it->second;

            unsigned int lastPosKey = 0, lastRotKey = 0, lastSclKey = 0;
            for (int f = 0; f < ra.frameCount; ++f)
            {
                double time = (double)f * ticksPerFrame;
                glm::vec3 pos = InterpolatePosition(time, channel, lastPosKey, bindPoses[boneIdx].translation);
                glm::quat rot = InterpolateRotation(time, channel, lastRotKey, bindPoses[boneIdx].rotation);
                glm::vec3 scale = InterpolateScale(time, channel, lastSclKey, bindPoses[boneIdx].scale);
                ra.framePoses[f * ra.boneCount + boneIdx].translation = pos;
                ra.framePoses[f * ra.boneCount + boneIdx].rotation = rot;
                ra.framePoses[f * ra.boneCount + boneIdx].scale = scale;
            }
        }
    }
}

void AssimpImporter::MergeMeshesByMaterial()
{
    CH_PROFILE_SCOPE("AssimpImporter::MergeMeshesByMaterial");
    if (m_Data.instances.empty() || m_Data.meshes.empty()) return;
    struct InstanceGroup { std::vector<int> instanceIndices; };
    std::unordered_map<int, InstanceGroup> groups;
    for (int i = 0; i < (int)m_Data.instances.size(); ++i)
    {
        int meshIdx = m_Data.instances[i].meshIndex;
        if (meshIdx >= 0 && meshIdx < (int)m_Data.meshes.size())
            groups[m_Data.meshes[meshIdx].materialIndex].instanceIndices.push_back(i);
    }

    std::pmr::vector<RawMesh> mergedMeshes(m_Data.meshes.get_allocator());
    std::pmr::vector<MeshInstance> mergedInstances(m_Data.instances.get_allocator());
    for (auto& [matIdx, group] : groups)
    {
        RawMesh merged(m_Data.meshes.get_allocator().resource());
        merged.materialIndex = matIdx;
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
                glm::vec4 pos = t * glm::vec4(src.vertices[v], src.vertices[v + 1], src.vertices[v + 2], 1.0f);
                merged.vertices.insert(merged.vertices.end(), {pos.x, pos.y, pos.z});
            }

            merged.texcoords.insert(merged.texcoords.end(), src.texcoords.begin(), src.texcoords.end());
            for (size_t n = 0; n < src.normals.size(); n += 3)
            {
                glm::vec3 norm = glm::normalize(normalMatrix * glm::vec3(src.normals[n], src.normals[n + 1], src.normals[n + 2]));
                merged.normals.insert(merged.normals.end(), {norm.x, norm.y, norm.z});
            }
            for (uint32_t idx : src.indices) merged.indices.push_back(idx + vertexOffset);
            for (size_t v = 0; v < merged.vertices.size(); v += 3)
            {
                glm::vec3 p = {merged.vertices[v], merged.vertices[v + 1], merged.vertices[v + 2]};
                merged.MinBounds = glm::min(merged.MinBounds, p);
                merged.MaxBounds = glm::max(merged.MaxBounds, p);
            }
        }
        mergedMeshes.push_back(std::move(merged));
        mergedInstances.push_back({(int)mergedMeshes.size() - 1, glm::mat4(1.0f)});
    }
    m_Data.meshes = std::move(mergedMeshes);
    m_Data.instances = std::move(mergedInstances);
}
} // namespace CHEngine
