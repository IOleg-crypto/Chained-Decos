#include "engine/assets/loaders/model_loader.h"
#include "engine/assets/loaders/assimp_importer.h"
#include "engine/assets/model_data.h"

#include "engine/assets/types/model_asset.h"
#include "engine/graphics/pipeline/geometry_generator.h"
#include "engine/common/zstd_compression.h"
#include "engine/core/service_locator.h"
#include "engine/assets/asset_manager.h"
#include "engine/assets/asset_metadata.h"
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <fstream>
#include <numeric>
#include <filesystem>

namespace Chained
{

std::shared_ptr<Asset> ModelLoader::Create()
{
    return std::make_shared<ModelAsset>();
}

bool ModelLoader::Load(std::shared_ptr<Asset> asset, const std::string& resolvedPath, std::string* outError)
{
    auto modelAsset = std::static_pointer_cast<ModelAsset>(asset);

    if (resolvedPath.starts_with(":"))
    {
        auto pendingData = GeometryGenerator::GeneratePrimitivePendingData(resolvedPath, ProceduralParameters());
        if (pendingData.isValid)
        {
            modelAsset->SetPendingData(std::move(pendingData));
            return true;
        }
        if (outError)
        {
            *outError = "ModelLoader: failed to generate procedural model for: " + resolvedPath;
        }
        return false;
    }

    auto pendingData = LoadMeshDataFromDisk(resolvedPath);
    if (pendingData.isValid)
    {
        modelAsset->SetPendingData(std::move(pendingData));
        return true;
    }
    if (outError)
    {
        *outError = "ModelLoader: failed to import model data from '" + resolvedPath + "'";
    }
    return false;
}

PendingModelData ModelLoader::LoadMeshDataFromDisk(const std::filesystem::path& path, int samplingFPS)
{
    std::filesystem::path chassetPath = path;
    chassetPath.replace_extension(".chasset");

    auto* am = ServiceLocator::TryGet<AssetManager>();

    std::vector<uint8_t> chassetBytes;
    if (am)
    {
        chassetBytes = am->ReadProjectAsset(chassetPath);
        if (chassetBytes.empty())
        {
            chassetBytes = am->ReadAssetData(chassetPath.generic_string());
        }
    }
    else if (std::filesystem::exists(chassetPath))
    {
        std::ifstream is(chassetPath, std::ios::binary | std::ios::ate);
        if (is.is_open())
        {
            auto sz = is.tellg();
            is.seekg(0);
            chassetBytes.resize(static_cast<size_t>(sz));
            is.read(reinterpret_cast<char*>(chassetBytes.data()), sz);
        }
    }

    if (!chassetBytes.empty())
    {
        try
        {
            std::string rawData(chassetBytes.begin(), chassetBytes.end());
            std::istringstream is(rawData, std::ios::binary);
            cereal::BinaryInputArchive archive(is);

            ChainedAssetHeader header;
            archive(header);

            ChainedAssetHeader currentHeader;

            if (header.magic != currentHeader.magic)
            {
                CH_CORE_WARN("Invalid .chasset file format (magic mismatch) for: {}", chassetPath.string());
            }
            else if (header.version != currentHeader.version)
            {
                CH_CORE_WARN("Engine data structure changed! .chasset is outdated for: {}", chassetPath.string());
            }
            else if (header.dataStructSize != sizeof(PendingModelData))
            {
                CH_CORE_WARN(".chasset struct size mismatch (got {}, expected {}), re-importing: {}",
                             header.dataStructSize, sizeof(PendingModelData), chassetPath.string());
            }
            else
            {
                bool hashValid = true;
                if (!am || !am->IsPacked())
                {
                    uint64_t currentHash = MetaUtils::ComputeFileHash(path);
                    if (currentHash != 0 && header.sourceHash != currentHash)
                    {
                        CH_CORE_WARN("Source file changed since .chasset was created, re-importing: {}", path.string());
                        hashValid = false;
                    }
                }

                if (hashValid)
                {
                    PendingModelData data;
                    if (header.compressed)
                    {
                        std::vector<char> compressedData(static_cast<size_t>(header.compressedSize));
                        is.read(compressedData.data(), static_cast<std::streamsize>(header.compressedSize));

                        auto decompressed =
                            Zstd::Decompress(compressedData.data(), compressedData.size(), header.uncompressedSize);

                        if (decompressed.empty())
                        {
                            CH_CORE_WARN("Failed to decompress .chasset, falling back to Assimp: {}",
                                         chassetPath.string());
                        }
                        else
                        {
                            std::istringstream dis(std::string(decompressed.begin(), decompressed.end()),
                                                   std::ios::binary);
                            cereal::BinaryInputArchive decompressedArchive(dis);
                            decompressedArchive(data);
                            return data;
                        }
                    }
                    else
                    {
                        archive(data);
                        return data;
                    }
                }
            }
        } catch (const std::bad_alloc& e)
        {
            CH_CORE_ERROR("Out of memory deserializing .chasset ({}), falling back to Assimp: {}", chassetPath.string(),
                          e.what());
        } catch (const std::exception& e)
        {
            CH_CORE_WARN("Failed to deserialize .chasset ({}), falling back to Assimp: {}", chassetPath.string(),
                         e.what());
        }
    }

    PendingModelData data = AssimpImporter::Import(path, samplingFPS);

    if (data.isValid && !path.string().starts_with(":"))
    {
        // Skip .chasset cache writes in packed/exported mode (directory may be read-only)
        auto* am = ServiceLocator::TryGet<AssetManager>();
        bool skipCache = am && am->IsPacked();

        if (!skipCache)
        {
            try
            {
                std::ostringstream dataStream(std::ios::binary);
                {
                    cereal::BinaryOutputArchive dataArchive(dataStream);
                    dataArchive(data);
                }

                std::string serializedData = dataStream.str();
                uint64_t sourceHash = MetaUtils::ComputeFileHash(path);

                auto compressed = Zstd::Compress(serializedData.data(), serializedData.size(), 3);

                ChainedAssetHeader header;
                header.sourceHash = sourceHash;
                header.compressed = !compressed.empty();
                header.compressedSize = compressed.size();
                header.uncompressedSize = serializedData.size();

                std::ofstream os(chassetPath, std::ios::binary);
                cereal::BinaryOutputArchive archive(os);
                archive(header);

                if (header.compressed)
                {
                    os.write(reinterpret_cast<const char*>(compressed.data()),
                             static_cast<std::streamsize>(compressed.size()));
                }
                else
                {
                    os.write(serializedData.data(), static_cast<std::streamsize>(serializedData.size()));
                }

                CH_CORE_INFO("ModelAsset: Saved .chasset '{}' (compressed: {}, ratio: {:.1f}%)",
                             chassetPath.filename().string(), header.compressed ? "yes" : "no",
                             header.compressed ? (100.0 * compressed.size() / serializedData.size()) : 100.0);
            } catch (const std::bad_alloc& e)
            {
                CH_CORE_ERROR("Out of memory serializing .chasset for {}: {}", chassetPath.string(), e.what());
            } catch (const std::exception& e)
            {
                CH_CORE_WARN("Failed to serialize .chasset for {}: {}", chassetPath.string(), e.what());
            }
        }
    }

    return data;
}
} // namespace Chained