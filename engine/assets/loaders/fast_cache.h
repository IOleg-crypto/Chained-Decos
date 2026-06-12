#ifndef CH_FAST_CACHE_H
#define CH_FAST_CACHE_H

#include "engine/core/log.h"
#include "engine/graphics/api/model_data.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Chained
{
/**
 * @brief Ultra-fast binary serialization for PendingModelData.
 * Uses raw memory dumps (POD-style) for maximum throughput.
 */
class FastCache
{
public:
    static constexpr uint32_t MAGIC = 0x4348434D; // "CHCM"
    static constexpr uint32_t VERSION = 1002;

    static bool Save(const std::filesystem::path& path, const PendingModelData& data)
    {
        std::ofstream os(path, std::ios::binary);
        if (!os.is_open())
        {
            return false;
        }

        Write(os, MAGIC);
        Write(os, VERSION);

        WriteString(os, data.fullPath);
        Write(os, data.nodeCount);
        Write(os, data.FinalizationProgress);
        Write(os, data.isValid);

        WriteVector(os, data.meshes);
        WriteVector(os, data.materials);
        WriteVector(os, data.instances);
        WriteVector(os, data.animations);
        WriteVector(os, data.bones);
        WriteVector(os, data.bindPose);
        WriteVector(os, data.nodeNames);
        WriteVector(os, data.nodeParents);
        WriteVector(os, data.nodeLocalTransforms);
        WriteVector(os, data.globalBindPoses);
        WriteVector(os, data.offsetMatrices);

        // Embedded textures
        uint32_t texCount = (uint32_t)data.embeddedTextures.size();
        Write(os, texCount);
        for (const auto& [name, tex] : data.embeddedTextures)
        {
            WriteString(os, name);
            Write(os, tex.width);
            Write(os, tex.height);
            Write(os, tex.channels);
            Write(os, tex.isHDR);
            WriteVector(os, tex.data);
        }

        return true;
    }

    static bool Load(const std::filesystem::path& path, PendingModelData& outData)
    {
        std::ifstream is(path, std::ios::binary);
        if (!is.is_open())
        {
            return false;
        }

        uint32_t magic = 0;
        uint32_t version = 0;
        Read(is, magic);
        Read(is, version);

        if (magic != MAGIC || version != VERSION)
        {
            return false;
        }

        ReadString(is, outData.fullPath);
        Read(is, outData.nodeCount);
        Read(is, outData.FinalizationProgress);
        Read(is, outData.isValid);

        ReadVector(is, outData.meshes);
        ReadVector(is, outData.materials);
        ReadVector(is, outData.instances);
        ReadVector(is, outData.animations);
        ReadVector(is, outData.bones);
        ReadVector(is, outData.bindPose);
        ReadVector(is, outData.nodeNames);
        ReadVector(is, outData.nodeParents);
        ReadVector(is, outData.nodeLocalTransforms);
        ReadVector(is, outData.globalBindPoses);
        ReadVector(is, outData.offsetMatrices);

        uint32_t texCount = 0;
        Read(is, texCount);
        outData.embeddedTextures.clear();
        for (uint32_t i = 0; i < texCount; ++i)
        {
            std::pmr::string name(outData.meshes.get_allocator().resource());
            ReadString(is, name);
            EmbeddedTextureData tex;
            Read(is, tex.width);
            Read(is, tex.height);
            Read(is, tex.channels);
            Read(is, tex.isHDR);
            ReadVector(is, tex.data);
            outData.embeddedTextures.emplace(name, std::move(tex));
        }

        return true;
    }

private:
    template <typename T> static void Write(std::ostream& os, const T& val)
    {
        os.write(reinterpret_cast<const char*>(&val), sizeof(T));
    }

    template <typename T> static void Read(std::istream& is, T& val)
    {
        is.read(reinterpret_cast<char*>(&val), sizeof(T));
    }

    static void WriteString(std::ostream& os, const std::pmr::string& str)
    {
        uint32_t size = (uint32_t)str.size();
        Write(os, size);
        os.write(str.data(), size);
    }

    static void ReadString(std::istream& is, std::pmr::string& str)
    {
        uint32_t size = 0;
        Read(is, size);
        str.resize(size);
        is.read(str.data(), size);
    }

    template <typename T, typename Alloc> static void WriteVector(std::ostream& os, const std::vector<T, Alloc>& vec)
    {
        uint32_t size = (uint32_t)vec.size();
        Write(os, size);
        if (size > 0)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                os.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
            }
            else
            {
                for (const auto& item : vec)
                {
                    SerializeItem(os, item);
                }
            }
        }
    }

    template <typename T, typename Alloc> static void ReadVector(std::istream& is, std::vector<T, Alloc>& vec)
    {
        uint32_t size = 0;
        Read(is, size);
        vec.resize(size);
        if (size > 0)
        {
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                is.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
            }
            else
            {
                for (auto& item : vec)
                {
                    DeserializeItem(is, item);
                }
            }
        }
    }

    // Specific serializers for non-trivial types
    static void SerializeItem(std::ostream& os, const RawMesh& m)
    {
        WriteVector(os, m.vertices);
        WriteVector(os, m.texcoords);
        WriteVector(os, m.normals);
        WriteVector(os, m.tangents);
        WriteVector(os, m.colors);
        WriteVector(os, m.indices);
        WriteVector(os, m.joints);
        WriteVector(os, m.weights);
        Write(os, m.materialIndex);
        Write(os, m.MinBounds);
        Write(os, m.MaxBounds);
    }

    static void DeserializeItem(std::istream& is, RawMesh& m)
    {
        ReadVector(is, m.vertices);
        ReadVector(is, m.texcoords);
        ReadVector(is, m.normals);
        ReadVector(is, m.tangents);
        ReadVector(is, m.colors);
        ReadVector(is, m.indices);
        ReadVector(is, m.joints);
        ReadVector(is, m.weights);
        Read(is, m.materialIndex);
        Read(is, m.MinBounds);
        Read(is, m.MaxBounds);
    }

    static void SerializeItem(std::ostream& os, const RawMaterial& m)
    {
        WriteString(os, m.albedoPath);
        Write(os, m.albedoColor);
        WriteString(os, m.emissivePath);
        Write(os, m.emissiveColor);
        Write(os, m.emissiveIntensity);
        WriteString(os, m.normalPath);
        WriteString(os, m.metallicRoughnessPath);
        WriteString(os, m.occlusionPath);
        Write(os, m.metalness);
        Write(os, m.roughness);
        Write(os, m.transparent);
    }

    static void DeserializeItem(std::istream& is, RawMaterial& m)
    {
        ReadString(is, m.albedoPath);
        Read(is, m.albedoColor);
        ReadString(is, m.emissivePath);
        Read(is, m.emissiveColor);
        Read(is, m.emissiveIntensity);
        ReadString(is, m.normalPath);
        ReadString(is, m.metallicRoughnessPath);
        ReadString(is, m.occlusionPath);
        Read(is, m.metalness);
        Read(is, m.roughness);
        Read(is, m.transparent);
    }

    static void SerializeItem(std::ostream& os, const RawAnimation& a)
    {
        WriteString(os, a.name);
        Write(os, a.frameCount);
        Write(os, a.boneCount);
        Write(os, a.frameRate);
        WriteVector(os, a.framePoses);
    }

    static void DeserializeItem(std::istream& is, RawAnimation& a)
    {
        ReadString(is, a.name);
        Read(is, a.frameCount);
        Read(is, a.boneCount);
        Read(is, a.frameRate);
        ReadVector(is, a.framePoses);
    }

    static void SerializeItem(std::ostream& os, const BoneInfoData& b)
    {
        WriteString(os, b.name);
        Write(os, b.parent);
    }

    static void DeserializeItem(std::istream& is, BoneInfoData& b)
    {
        ReadString(is, b.name);
        Read(is, b.parent);
    }

    template <typename T> static void SerializeItem(std::ostream& os, const T& val)
    {
        Write(os, val);
    }
    template <typename T> static void DeserializeItem(std::istream& is, T& val)
    {
        Read(is, val);
    }
};
} // namespace Chained
#endif // CH_FAST_CACHE_H