#include "model_cache.h"
#include <fstream>

namespace CHEngine
{
    namespace
    {
        constexpr uint32_t kCacheVersion = 3;

        template<typename T>
        void WritePOD(std::ostream& os, const T& value)
        {
            os.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        template<typename T>
        void ReadPOD(std::istream& is, T& value)
        {
            is.read(reinterpret_cast<char*>(&value), sizeof(T));
        }

        void WriteString(std::ostream& os, const std::string& str)
        {
            uint32_t size = static_cast<uint32_t>(str.size());
            WritePOD(os, size);
            if (size > 0)
                os.write(str.data(), size);
        }

        void ReadString(std::istream& is, std::string& str)
        {
            uint32_t size = 0;
            ReadPOD(is, size);
            str.resize(size);
            if (size > 0)
                is.read(str.data(), size);
        }

        template<typename T>
        void WriteVector(std::ostream& os, const std::vector<T>& vec)
        {
            uint32_t size = static_cast<uint32_t>(vec.size());
            WritePOD(os, size);
            if (size > 0)
                os.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
        }

        template<typename T>
        void ReadVector(std::istream& is, std::vector<T>& vec)
        {
            uint32_t size = 0;
            ReadPOD(is, size);
            vec.resize(size);
            if (size > 0)
                is.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
        }

        void WriteEmbeddedTextures(std::ostream& os, const std::unordered_map<std::string, EmbeddedTextureData>& textures)
        {
            WritePOD(os, static_cast<uint32_t>(textures.size()));
            for (const auto& [name, tex] : textures)
            {
                WriteString(os, name);
                WriteVector(os, tex.data);
                WritePOD(os, tex.width);
                WritePOD(os, tex.height);
                WritePOD(os, tex.channels);
                WritePOD(os, tex.isHDR);
            }
        }

        void ReadEmbeddedTextures(std::istream& is, std::unordered_map<std::string, EmbeddedTextureData>& textures)
        {
            uint32_t count = 0;
            ReadPOD(is, count);
            textures.clear();
            for (uint32_t i = 0; i < count; ++i)
            {
                std::string name;
                ReadString(is, name);
                EmbeddedTextureData tex;
                ReadVector(is, tex.data);
                ReadPOD(is, tex.width);
                ReadPOD(is, tex.height);
                ReadPOD(is, tex.channels);
                ReadPOD(is, tex.isHDR);
                textures[name] = std::move(tex);
            }
        }
    }

    bool ModelCache::Save(const std::filesystem::path& cachePath, const PendingModelData& data)
    {
        std::ofstream os(cachePath, std::ios::binary);
        if (!os) return false;

        WritePOD(os, kCacheVersion);
        WriteString(os, data.fullPath);

        // Meshes
        WritePOD(os, static_cast<uint32_t>(data.meshes.size()));
        for (const auto& mesh : data.meshes)
        {
            WriteVector(os, mesh.vertices);
            WriteVector(os, mesh.texcoords);
            WriteVector(os, mesh.normals);
            WriteVector(os, mesh.tangents);
            WriteVector(os, mesh.colors);
            WriteVector(os, mesh.indices);
            WriteVector(os, mesh.joints);
            WriteVector(os, mesh.weights);
            WritePOD(os, mesh.materialIndex);
            WritePOD(os, mesh.MinBounds);
            WritePOD(os, mesh.MaxBounds);
        }

        // Materials
        WritePOD(os, static_cast<uint32_t>(data.materials.size()));
        for (const auto& mat : data.materials)
        {
            WriteString(os, mat.albedoPath);
            WritePOD(os, mat.albedoColor);
            WriteString(os, mat.emissivePath);
            WritePOD(os, mat.emissiveColor);
            WritePOD(os, mat.emissiveIntensity);
            WriteString(os, mat.normalPath);
            WriteString(os, mat.metallicRoughnessPath);
            WriteString(os, mat.occlusionPath);
            WritePOD(os, mat.metalness);
            WritePOD(os, mat.roughness);
            WritePOD(os, mat.transparent);
        }

        // Embedded Textures
        WriteEmbeddedTextures(os, data.embeddedTextures);

        // Skeletal Data
        WriteVector(os, data.bones);
        WriteVector(os, data.bindPose);

        // Instances
        WriteVector(os, data.instances);

        // Hierarchy
        WritePOD(os, data.nodeCount);
        WritePOD(os, static_cast<uint32_t>(data.nodeNames.size()));
        for (const auto& name : data.nodeNames) WriteString(os, name);
        WriteVector(os, data.nodeParents);
        WriteVector(os, data.nodeLocalTransforms);
        WriteVector(os, data.globalBindPoses);
        WriteVector(os, data.offsetMatrices);

        // Animations
        WritePOD(os, static_cast<uint32_t>(data.animations.size()));
        for (const auto& anim : data.animations)
        {
            WriteString(os, anim.name);
            WritePOD(os, anim.frameCount);
            WritePOD(os, anim.boneCount);
            WritePOD(os, anim.frameRate);
            WriteVector(os, anim.framePoses);
        }

        return true;
    }

    bool ModelCache::Load(const std::filesystem::path& cachePath, PendingModelData& data)
    {
        std::ifstream is(cachePath, std::ios::binary);
        if (!is) return false;

        uint32_t version = 0;
        ReadPOD(is, version);
        if (version != kCacheVersion) return false;

        ReadString(is, data.fullPath);

        // Meshes
        uint32_t meshCount = 0;
        ReadPOD(is, meshCount);
        data.meshes.resize(meshCount);
        for (auto& mesh : data.meshes)
        {
            ReadVector(is, mesh.vertices);
            ReadVector(is, mesh.texcoords);
            ReadVector(is, mesh.normals);
            ReadVector(is, mesh.tangents);
            ReadVector(is, mesh.colors);
            ReadVector(is, mesh.indices);
            ReadVector(is, mesh.joints);
            ReadVector(is, mesh.weights);
            ReadPOD(is, mesh.materialIndex);
            ReadPOD(is, mesh.MinBounds);
            ReadPOD(is, mesh.MaxBounds);
        }

        // Materials
        uint32_t matCount = 0;
        ReadPOD(is, matCount);
        data.materials.resize(matCount);
        for (auto& mat : data.materials)
        {
            ReadString(is, mat.albedoPath);
            ReadPOD(is, mat.albedoColor);
            ReadString(is, mat.emissivePath);
            ReadPOD(is, mat.emissiveColor);
            ReadPOD(is, mat.emissiveIntensity);
            ReadString(is, mat.normalPath);
            ReadString(is, mat.metallicRoughnessPath);
            ReadString(is, mat.occlusionPath);
            ReadPOD(is, mat.metalness);
            ReadPOD(is, mat.roughness);
            ReadPOD(is, mat.transparent);
        }

        // Embedded Textures
        ReadEmbeddedTextures(is, data.embeddedTextures);

        // Skeletal Data
        ReadVector(is, data.bones);
        ReadVector(is, data.bindPose);

        // Instances
        ReadVector(is, data.instances);

        // Hierarchy
        ReadPOD(is, data.nodeCount);
        uint32_t nodeNameCount = 0;
        ReadPOD(is, nodeNameCount);
        data.nodeNames.resize(nodeNameCount);
        for (auto& name : data.nodeNames) ReadString(is, name);
        ReadVector(is, data.nodeParents);
        ReadVector(is, data.nodeLocalTransforms);
        ReadVector(is, data.globalBindPoses);
        ReadVector(is, data.offsetMatrices);

        // Animations
        uint32_t animCount = 0;
        ReadPOD(is, animCount);
        data.animations.resize(animCount);
        for (auto& anim : data.animations)
        {
            ReadString(is, anim.name);
            ReadPOD(is, anim.frameCount);
            ReadPOD(is, anim.boneCount);
            ReadPOD(is, anim.frameRate);
            ReadVector(is, anim.framePoses);
        }

        data.isValid = true;
        return true;
    }

    std::filesystem::path ModelCache::GetCachePath(const std::filesystem::path& modelPath)
    {
        return modelPath.string() + ".chcache";
    }

    bool ModelCache::IsCacheValid(const std::filesystem::path& modelPath)
    {
        auto cachePath = GetCachePath(modelPath);
        if (!std::filesystem::exists(cachePath)) return false;

        auto modelTime = std::filesystem::last_write_time(modelPath);
        auto cacheTime = std::filesystem::last_write_time(cachePath);

        return cacheTime >= modelTime;
    }
}
