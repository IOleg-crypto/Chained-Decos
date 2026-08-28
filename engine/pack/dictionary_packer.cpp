#include "dictionary_packer.h"

#include "engine/core/log.h"

extern "C" {
#include <pack/common.h>
#include <zstd.h>
#include <zdict.h>
}

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace Chained
{
	struct ItemMeta
	{
		std::string ItemPath;
		std::string DiskFilePath;
		uint32_t DataSize = 0;
	};

	bool DictionaryPacker::Pack(const fs::path& packPath, const std::vector<DictionaryPackItem>& items,
								uint32_t dataVersion, float zipThreshold, bool printProgress,
								const DictionaryPackProgressCallback& onProgress, int compressionLevel)
	{
		if (items.empty())
		{
			CH_CORE_ERROR("DictionaryPacker: No items to pack");
			return false;
		}

		// Step 1: Scan file sizes from disk (0 RAM consumption)
		if (printProgress)
		{
			printf("DictionaryPacker: Scanning %zu files...\n", items.size());
			fflush(stdout);
		}

		std::vector<ItemMeta> itemMetaList(items.size());
		uint64_t totalRawSize = 0;

		for (size_t i = 0; i < items.size(); i++)
		{
			itemMetaList[i].ItemPath = items[i].ItemPath;
			itemMetaList[i].DiskFilePath = items[i].FilePath.string();

			std::error_code ec;
			itemMetaList[i].DataSize = (uint32_t)fs::file_size(items[i].FilePath, ec);
			if (!ec)
			{
				totalRawSize += itemMetaList[i].DataSize;
			}
		}

		// Step 2: Sort items by path length and name (Pack format standard)
		std::sort(itemMetaList.begin(), itemMetaList.end(), [](const ItemMeta& a, const ItemMeta& b) {
			size_t al = a.ItemPath.size();
			size_t bl = b.ItemPath.size();
			if (al != bl)
			{
				return al < bl;
			}
			return a.ItemPath < b.ItemPath;
		});

		// Step 3: Train ZSTD dictionary from sampled files read on demand (max 64 MB sample)
		if (printProgress)
		{
			printf("DictionaryPacker: Training ZSTD dictionary...\n");
			fflush(stdout);
		}

		static constexpr size_t kMaxSampleBytes = 64 * 1024 * 1024; // 64 MB
		std::vector<uint8_t> samplesBuffer;
		std::vector<size_t> sampleSizes;
		samplesBuffer.reserve(std::min(totalRawSize, kMaxSampleBytes));

		std::vector<size_t> sampleIndices(itemMetaList.size());
		std::iota(sampleIndices.begin(), sampleIndices.end(), 0);
		for (size_t k = sampleIndices.size(); k > 1; k--)
		{
			std::swap(sampleIndices[k - 1], sampleIndices[(k * 2654435761ULL) % k]);
		}

		size_t sampledBytes = 0;
		for (size_t idx : sampleIndices)
		{
			const ItemMeta& item = itemMetaList[idx];
			if (item.DataSize == 0)
			{
				continue;
			}
			if (sampledBytes + item.DataSize > kMaxSampleBytes)
			{
				break;
			}

			FILE* sf = fopen(item.DiskFilePath.c_str(), "rb");
			if (!sf)
			{
				continue;
			}

			size_t oldSize = samplesBuffer.size();
			samplesBuffer.resize(oldSize + item.DataSize);
			size_t r = fread(samplesBuffer.data() + oldSize, 1, item.DataSize, sf);
			fclose(sf);

			if (r == item.DataSize)
			{
				sampleSizes.push_back(item.DataSize);
				sampledBytes += item.DataSize;
			}
			else
			{
				samplesBuffer.resize(oldSize);
			}
		}

		// Dictionary size: 1% of total data, clamped between 100KB and 4MB.
		size_t dictCapacity = std::max<size_t>(100 * 1024, totalRawSize / 100);
		dictCapacity = std::min(dictCapacity, (size_t)1 << 22); // max 4MB

		std::vector<uint8_t> dictBuffer(dictCapacity);
		size_t dictSize = 0;

		if (!sampleSizes.empty())
		{
			dictSize = ZDICT_trainFromBuffer(dictBuffer.data(), dictCapacity, samplesBuffer.data(), sampleSizes.data(),
											 (unsigned)sampleSizes.size());
		}

		// Free sample training buffer immediately
		samplesBuffer.clear();
		samplesBuffer.shrink_to_fit();

		if (ZDICT_isError(dictSize))
		{
			CH_CORE_WARN("DictionaryPacker: Dictionary training warning: {}", ZDICT_getErrorName(dictSize));
			dictSize = 0;
		}

		if (printProgress && dictSize > 0)
		{
			printf("DictionaryPacker: Dictionary trained (%zu bytes)\n", dictSize);
			fflush(stdout);
		}

		// Step 4: Create CDict and CCtx
		ZSTD_CDict* cdict = nullptr;
		compressionLevel = std::max(1, std::min(compressionLevel, ZSTD_maxCLevel()));

		if (dictSize > 0)
		{
			cdict = ZSTD_createCDict(dictBuffer.data(), dictSize, compressionLevel);
		}

		// Step 5: Open pack file and write header + dictionary
		FILE* packFile = fopen(packPath.string().c_str(), "wb");
		if (!packFile)
		{
			CH_CORE_ERROR("DictionaryPacker: Cannot create file '{}'", packPath.string());
			if (cdict)
			{
				ZSTD_freeCDict(cdict);
			}
			return false;
		}

		PackHeader header;
		header.magic = PACK_HEADER_MAGIC;
		header.versionMajor = PACK_VERSION_MAJOR;
		header.versionMinor = PACK_VERSION_MINOR;
		header.versionPatch = PACK_VERSION_PATCH;
		header.isBigEndian = !PACK_LITTLE_ENDIAN;
		header.itemCount = itemMetaList.size();
		header.dataVersion = dataVersion;
		header.preferSpeed = 0;
		header._reserved = dictSize > 0 ? 1 : 0; // flag: has dictionary

		if (fwrite(&header, sizeof(PackHeader), 1, packFile) != 1)
		{
			fclose(packFile);
			if (cdict)
			{
				ZSTD_freeCDict(cdict);
			}
			return false;
		}

		uint64_t fileOffset = sizeof(PackHeader);

		// Write dictionary as first item if available
		if (dictSize > 0)
		{
			const char* dictPath = DictionaryPacker::DictionaryItemPath;
			uint8_t pathSize = (uint8_t)strlen(dictPath);

			PackItemHeader itemHeader;
			itemHeader.dataSize = (uint32_t)dictSize;
			itemHeader.zipSize = 0; // dictionary stored raw
			itemHeader.pathSize = pathSize;
			itemHeader.isReference = 0;
			itemHeader.dataOffset = fileOffset + sizeof(PackItemHeader) + pathSize;

			if (fwrite(&itemHeader, sizeof(PackItemHeader), 1, packFile) != 1 ||
				fwrite(dictPath, sizeof(char), pathSize, packFile) != pathSize ||
				fwrite(dictBuffer.data(), sizeof(uint8_t), dictSize, packFile) != dictSize)
			{
				fclose(packFile);
				if (cdict)
				{
					ZSTD_freeCDict(cdict);
				}
				return false;
			}

			fileOffset += sizeof(PackItemHeader) + pathSize + dictSize;
		}

		// Step 6: Multi-threaded batch streaming compression
		// Compresses small batches of files in parallel across all CPU cores (<60 MB peak RAM)
		if (printProgress)
		{
			printf("DictionaryPacker: Compressing and streaming %zu files to pack in parallel...\n",
				   itemMetaList.size());
			fflush(stdout);
		}

		struct CompressedItem
		{
			uint32_t ZipSize = 0;
			std::vector<uint8_t> Data;
		};

		const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency() - 1);
		const size_t batchSize = std::max<size_t>(16, threadCount * 2);

		for (size_t batchStart = 0; batchStart < itemMetaList.size(); batchStart += batchSize)
		{
			size_t batchEnd = std::min(batchStart + batchSize, itemMetaList.size());
			size_t currentBatchCount = batchEnd - batchStart;

			std::vector<CompressedItem> batchResults(currentBatchCount);

			// Parallel compression of this batch
			std::atomic<size_t> nextBatchItem{0};
			std::vector<std::thread> workers;
			workers.reserve(threadCount);

			for (unsigned int t = 0; t < threadCount; ++t)
			{
				workers.emplace_back([&]() {
					ZSTD_CCtx* localCtx = ZSTD_createCCtx();
					std::vector<uint8_t> rawBuf;
					std::vector<uint8_t> compBuf;

					while (true)
					{
						size_t localIdx = nextBatchItem.fetch_add(1, std::memory_order_relaxed);
						if (localIdx >= currentBatchCount)
						{
							break;
						}

						size_t globalIdx = batchStart + localIdx;
						const ItemMeta& item = itemMetaList[globalIdx];
						CompressedItem& result = batchResults[localIdx];

						if (item.DataSize == 0)
						{
							result.ZipSize = 0;
							continue;
						}

						rawBuf.resize(item.DataSize);
						FILE* srcF = fopen(item.DiskFilePath.c_str(), "rb");
						if (!srcF)
						{
							result.ZipSize = 0;
							continue;
						}
						size_t bytesRead = fread(rawBuf.data(), 1, item.DataSize, srcF);
						fclose(srcF);

						if (bytesRead != item.DataSize)
						{
							result.ZipSize = 0;
							result.Data = std::move(rawBuf);
							continue;
						}

						size_t bound = ZSTD_compressBound(item.DataSize);
						compBuf.resize(bound);

						size_t compResult;
						if (cdict)
						{
							compResult = ZSTD_compress_usingCDict(localCtx, compBuf.data(), bound, rawBuf.data(),
																  item.DataSize, cdict);
						}
						else
						{
							compResult = ZSTD_compressCCtx(localCtx, compBuf.data(), bound, rawBuf.data(),
														   item.DataSize, compressionLevel);
						}

						const float maxAllowedRatio =
							(zipThreshold <= 0.0f || zipThreshold >= 1.0f) ? 1.0f : (1.0f - zipThreshold);
						if (!ZSTD_isError(compResult) && compResult < (size_t)(item.DataSize * maxAllowedRatio))
						{
							result.ZipSize = (uint32_t)compResult;
							compBuf.resize(compResult);
							result.Data = std::move(compBuf);
						}
						else
						{
							result.ZipSize = 0;
							result.Data = std::move(rawBuf);
						}
					}

					ZSTD_freeCCtx(localCtx);
				});
			}

			for (auto& w : workers)
			{
				w.join();
			}

			// Sequentially write the compressed batch to the pack file
			for (size_t localIdx = 0; localIdx < currentBatchCount; ++localIdx)
			{
				size_t globalIdx = batchStart + localIdx;
				const ItemMeta& item = itemMetaList[globalIdx];
				const CompressedItem& result = batchResults[localIdx];
				uint8_t pathSize = (uint8_t)item.ItemPath.size();

				PackItemHeader itemHeader;
				itemHeader.dataSize = item.DataSize;
				itemHeader.zipSize = result.ZipSize;
				itemHeader.pathSize = pathSize;
				itemHeader.isReference = 0;
				itemHeader.dataOffset = fileOffset + sizeof(PackItemHeader) + pathSize;

				if (fwrite(&itemHeader, sizeof(PackItemHeader), 1, packFile) != 1 ||
					fwrite(item.ItemPath.c_str(), sizeof(char), pathSize, packFile) != pathSize)
				{
					fclose(packFile);
					if (cdict)
					{
						ZSTD_freeCDict(cdict);
					}
					return false;
				}

				if (result.ZipSize > 0)
				{
					if (fwrite(result.Data.data(), sizeof(uint8_t), result.ZipSize, packFile) != result.ZipSize)
					{
						fclose(packFile);
						if (cdict)
						{
							ZSTD_freeCDict(cdict);
						}
						return false;
					}
					fileOffset += sizeof(PackItemHeader) + pathSize + result.ZipSize;
				}
				else if (item.DataSize > 0 && !result.Data.empty())
				{
					if (fwrite(result.Data.data(), sizeof(uint8_t), item.DataSize, packFile) != item.DataSize)
					{
						fclose(packFile);
						if (cdict)
						{
							ZSTD_freeCDict(cdict);
						}
						return false;
					}
					fileOffset += sizeof(PackItemHeader) + pathSize + item.DataSize;
				}
				else
				{
					fileOffset += sizeof(PackItemHeader) + pathSize;
				}

				if (onProgress)
				{
					onProgress(globalIdx + 1, itemMetaList.size(), item.ItemPath);
				}
			}

			if (printProgress)
			{
				int progress = (int)(((float)batchEnd / (float)itemMetaList.size()) * 100.0f);
				printf("[%3d%%] Packed %zu/%zu files in parallel...\n", progress, batchEnd, itemMetaList.size());
				fflush(stdout);
			}
		}

		fclose(packFile);

		if (cdict)
		{
			ZSTD_freeCDict(cdict);
		}

		if (printProgress)
		{
			uint64_t packSize = fs::file_size(packPath);
			int compression = totalRawSize > 0 ? (int)((1.0 - (double)packSize / (double)totalRawSize) * 100.0) : 0;
			printf("Packed %zu files. (%llu/%llu bytes, %d%% saved)\n", items.size(), (unsigned long long)packSize,
				   (unsigned long long)totalRawSize, compression);
		}

		return true;
	}
} // namespace Chained
