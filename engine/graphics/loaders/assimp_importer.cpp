#include "engine/graphics/loaders/assimp_importer.h"
#include "engine/core/log.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stb_image.h>
#include <mutex>
#include <fstream>
#include <cstring>

namespace CHEngine
{
    static std::mutex s_AssimpMutex;
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

    // --- Minimalist Conversion Helpers ---
    static glm::mat4 ToMat4(const aiMatrix4x4& m) {
        // Assimp stores matrices in row-major memory.
        // glm::make_mat4 reads 16 floats and treats them as 4 columns (column-major).
        // If we don't transpose, we get the transpose of the intended matrix (translation in the last row).
        // Thus, we must transpose to get the logical matrix into GLM's column-major memory.
        return glm::transpose(glm::make_mat4(&m.a1)); 
    }

    static glm::vec3 ToVec3(const aiVector3D& v) { return { v.x, v.y, v.z }; }
    static glm::vec2 ToVec2(const aiVector3D& v) { return { v.x, v.y }; }
    static glm::quat ToQuat(const aiQuaternion& q) { return { q.w, q.x, q.y, q.z }; }
    static glm::vec4 ToColor(const aiColor4D& c) { return { c.r, c.g, c.b, c.a }; }

    PendingModelData AssimpImporter::Import(const std::filesystem::path& path, int samplingFPS)
    {
        Assimp::Importer importer;
        std::string exts;
        importer.GetExtensionList(exts);
        CH_CORE_INFO("AssimpImporter: Supported extensions: {0}", exts);
        
        CH_CORE_INFO("AssimpImporter: Attempting to import '{0}'", path.string());
        std::lock_guard<std::mutex> lock(s_AssimpMutex);
        PendingModelData data{};
        
        // Match flags and properties from the develop branch for consistency
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
        importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65535);

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)std::tolower(c); });
        if (!IsSupportedAssimpExtension(ext))
        {
            CH_CORE_ERROR("Assimp Model Load Failed: {} | Error: unsupported format '{}'. Supported formats: glTF/GLB, OBJ", path.filename().string(), ext);
            return data;
        }

        unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                             aiProcess_LimitBoneWeights | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType |
                             aiProcess_CalcTangentSpace | aiProcess_SplitLargeMeshes |
                             aiProcess_ImproveCacheLocality | aiProcess_ValidateDataStructure |
                             aiProcess_FindInvalidData;

        // GLTF/GLB already stores UVs with the correct top-left origin for OpenGL sampling.
        // Adding aiProcess_FlipUVs would flip them a SECOND time → upside-down textures.
        // Only apply it for formats that need it (OBJ, FBX, etc.).
        if (ext != ".gltf" && ext != ".glb")
        {
            flags |= aiProcess_FlipUVs;
        }

        const aiScene* scene = nullptr;

        CH_CORE_INFO("AssimpImporter: Invoking ReadFile for '{0}'", path.string());
        scene = importer.ReadFile(path.string(), flags);

        if (!scene || !scene->mRootNode)
        {
            CH_CORE_WARN("AssimpImporter: ReadFile failed for '{0}': {1}. Trying memory loading as fallback...", path.string(), importer.GetErrorString());
            
            // "Robust-Light" Loading Strategy: Read to memory with padding to avoid corruption issues
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (file.is_open())
            {
                std::streamsize size = file.tellg();
                if (size > 0)
                {
                    file.seekg(0);
                    std::vector<char> buffer(static_cast<size_t>(size) + 1024, 0); 
                    file.read(buffer.data(), size);
                    scene = importer.ReadFileFromMemory(buffer.data(), static_cast<size_t>(size), flags, path.extension().string().c_str());
                }
            }
        }

        if (!scene) {
            CH_CORE_ERROR("Assimp Model Load Failed: {} | Error: {}", path.filename().string(), importer.GetErrorString());
            return data;
        }

        if (!scene->mRootNode)
        {
            CH_CORE_ERROR("Assimp Model Load Failed: {} | Error: scene has no root node", path.filename().string());
            return data;
        }

        // --- 1. Process Hierarchy & Bind Poses ---
        std::map<std::string, int> nameToIndex;
        std::filesystem::path modelDir = path.parent_path();

        std::function<void(aiNode*, int)> processNode = [&](aiNode* node, int parentIdx) -> void {
            if (!node)
            {
                return;
            }

            int currentIdx = (int)data.nodeNames.size();
            data.nodeNames.push_back(node->mName.C_Str());
            data.nodeParents.push_back(parentIdx);
            
            glm::mat4 local = ToMat4(node->mTransformation);
            data.nodeLocalTransforms.push_back(local);
            data.globalBindPoses.push_back((parentIdx == -1) ? local : data.globalBindPoses[parentIdx] * local);

            for (unsigned int i = 0; i < node->mNumMeshes; ++i)
            {
                unsigned int meshIndex = node->mMeshes[i];
                if (meshIndex < scene->mNumMeshes)
                {
                    data.instances.push_back({ (int)meshIndex, data.globalBindPoses[currentIdx] });
                }
            }

            for (unsigned int i = 0; i < node->mNumChildren; ++i)
                processNode(node->mChildren[i], currentIdx);
        };
        processNode(scene->mRootNode, -1);
        data.nodeCount = (int)data.nodeNames.size();

        // --- 2. Process Meshes ---
        data.meshes.resize(scene->mNumMeshes);
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
            aiMesh* am = scene->mMeshes[m];
            RawMesh& rm = data.meshes[m];
            rm.materialIndex = am->mMaterialIndex;

            for (unsigned int v = 0; v < am->mNumVertices; ++v) {
                rm.vertices.insert(rm.vertices.end(), { am->mVertices[v].x, am->mVertices[v].y, am->mVertices[v].z });
                if (am->mTextureCoords[0]) rm.texcoords.insert(rm.texcoords.end(), { am->mTextureCoords[0][v].x, am->mTextureCoords[0][v].y });
                if (am->mNormals) rm.normals.insert(rm.normals.end(), { am->mNormals[v].x, am->mNormals[v].y, am->mNormals[v].z });
            }

            int triCount = 0;
            for (unsigned int f = 0; f < am->mNumFaces; ++f)
            {
                const aiFace& face = am->mFaces[f];
                if (face.mNumIndices != 3)
                {
                    continue;
                }

                rm.indices.insert(rm.indices.end(), { face.mIndices[0], face.mIndices[1], face.mIndices[2] });
                triCount++;
            }

            CH_CORE_INFO("AssimpImporter: Mesh '{}' loaded with {} vertices and {} triangles", am->mName.C_Str(), am->mNumVertices, triCount);

            // Bones & Weights
            if (am->mNumBones > 0) {
                rm.joints.resize(am->mNumVertices * 4, 0);
                rm.weights.resize(am->mNumVertices * 4, 0.0f);
                std::vector<int> jointCounts(am->mNumVertices, 0);

                for (unsigned int b = 0; b < am->mNumBones; ++b) {
                    aiBone* bone = am->mBones[b];
                    int boneIdx = -1;
                    for (int n = 0; n < (int)data.nodeNames.size(); ++n) if (data.nodeNames[n] == bone->mName.C_Str()) { boneIdx = n; break; }
                    
                    if (boneIdx != -1) {
                        if (boneIdx >= (int)data.offsetMatrices.size()) data.offsetMatrices.resize(boneIdx + 1);
                        data.offsetMatrices[boneIdx] = ToMat4(bone->mOffsetMatrix);

                        for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                            int vIdx = bone->mWeights[w].mVertexId;
                            if (vIdx < 0 || vIdx >= (int)am->mNumVertices)
                            {
                                continue;
                            }

                            if (jointCounts[vIdx] < 4) {
                                int slot = vIdx * 4 + jointCounts[vIdx]++;
                                rm.joints[slot] = (unsigned char)boneIdx;
                                rm.weights[slot] = bone->mWeights[w].mWeight;
                            }
                        }
                    }
                }
            }
        }

        // --- 3. Process Materials ---
        data.materials.resize(scene->mNumMaterials);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            aiMaterial* am = scene->mMaterials[i];
            RawMaterial& rm = data.materials[i];
            
            aiColor4D col(1.0f, 1.0f, 1.0f, 1.0f);
            bool colorFound = false;
            if (aiGetMaterialColor(am, AI_MATKEY_BASE_COLOR, &col) == AI_SUCCESS) {
                rm.albedoColor = ToColor(col);
                colorFound = true;
            } else if (aiGetMaterialColor(am, AI_MATKEY_COLOR_DIFFUSE, &col) == AI_SUCCESS) {
                rm.albedoColor = ToColor(col);
                colorFound = true;
            }

            // Safety fallback for transparency: if alpha is exactly 0 but we found a color, default it to 1.0
            // unless opacity key explicitly says otherwise.
            if (colorFound && rm.albedoColor.a < 0.001f) {
                rm.albedoColor.a = 1.0f;
            }

            float opacity = 1.0f;
            if (aiGetMaterialFloat(am, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS)
                rm.albedoColor.a *= opacity;

            if (aiGetMaterialColor(am, AI_MATKEY_COLOR_EMISSIVE, &col) == AI_SUCCESS)
                rm.emissiveColor = ToColor(col);

            aiGetMaterialFloat(am, AI_MATKEY_EMISSIVE_INTENSITY, &rm.emissiveIntensity);
            aiGetMaterialFloat(am, AI_MATKEY_METALLIC_FACTOR, &rm.metalness);
            aiGetMaterialFloat(am, AI_MATKEY_ROUGHNESS_FACTOR, &rm.roughness);

            auto resolvePath = [&](const std::string& texPath) -> std::string {
                if (texPath.empty()) return "";
                
                // 1. Try absolute / relative to CWD
                if (std::filesystem::exists(texPath)) return texPath;

                // 2. Try relative to model directory
                std::filesystem::path p1 = modelDir / texPath;
                if (std::filesystem::exists(p1)) return p1.string();

                // 3. Try filename only in model directory
                std::string filename = std::filesystem::path(texPath).filename().string();
                std::filesystem::path p2 = modelDir / filename;
                if (std::filesystem::exists(p2)) return p2.string();

                // 4. Try in a 'textures' subfolder
                std::filesystem::path p3 = modelDir / "textures" / filename;
                if (std::filesystem::exists(p3)) return p3.string();

                return texPath; // Fallback to original
            };

            auto getTex = [&](aiTextureType type) -> std::string {
                aiString str;
                if (am->GetTexture(type, 0, &str) == AI_SUCCESS) return resolvePath(str.C_Str());
                return "";
            };
            rm.albedoPath = getTex(aiTextureType_DIFFUSE);
            if (rm.albedoPath.empty()) rm.albedoPath = getTex(aiTextureType_BASE_COLOR); // GLTF often uses this
            rm.normalPath = getTex(aiTextureType_NORMALS);
            rm.metallicRoughnessPath = getTex(aiTextureType_METALNESS);
            if (rm.metallicRoughnessPath.empty()) rm.metallicRoughnessPath = getTex(aiTextureType_UNKNOWN); // GLTF MR is often here
        }

        // --- 4. Decode Embedded Textures ---
        for (unsigned int i = 0; i < scene->mNumTextures; ++i)
        {
            const aiTexture* texture = scene->mTextures[i];
            if (!texture)
            {
                continue;
            }

            EmbeddedTextureData embedded;
            if (!DecodeEmbeddedTexture(texture, embedded))
            {
                continue;
            }

            data.embeddedTextures.emplace("*" + std::to_string(i), std::move(embedded));
        }

        data.isValid = true;
        return data;
    }
}
