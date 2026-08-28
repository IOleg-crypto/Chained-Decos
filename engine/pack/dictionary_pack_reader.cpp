#include "dictionary_pack_reader.h"

#include "engine/core/log.h"

extern "C" {
#include <pack/common.h>
#include <zstd.h>
}

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace Chained
{

	DictionaryPackReader::~DictionaryPackReader()
	{
		Close();
	}

	DictionaryPackReader::DictionaryPackReader(DictionaryPackReader&& other) noexcept
		: m_File(other.m_File),
		  m_ItemCount(other.m_ItemCount),
		  m_DataVersion(other.m_DataVersion),
		  m_PreferSpeed(other.m_PreferSpeed),
		  m_HasDict(other.m_HasDict),
		  m_DDict(other.m_DDict),
		  m_DCtx(other.m_DCtx),
		  m_Items(std::move(other.m_Items))
	{
		other.m_File = nullptr;
		other.m_DDict = nullptr;
		other.m_DCtx = nullptr;
		other.m_ItemCount = 0;
	}

	DictionaryPackReader& DictionaryPackReader::operator=(DictionaryPackReader&& other) noexcept
	{
		if (this != &other)
		{
			Close();
			m_File = other.m_File;
			m_ItemCount = other.m_ItemCount;
			m_DataVersion = other.m_DataVersion;
			m_PreferSpeed = other.m_PreferSpeed;
			m_HasDict = other.m_HasDict;
			m_DDict = other.m_DDict;
			m_DCtx = other.m_DCtx;
			m_Items = std::move(other.m_Items);

			other.m_File = nullptr;
			other.m_DDict = nullptr;
			other.m_DCtx = nullptr;
			other.m_ItemCount = 0;
		}
		return *this;
	}

	bool DictionaryPackReader::Open(const fs::path& packPath)
	{
		Close();

		m_File = fopen(packPath.string().c_str(), "rb");
		if (!m_File)
		{
			CH_CORE_ERROR("DictionaryPackReader: Cannot open '{}'", packPath.string());
			return false;
		}

		if (!ReadHeader())
		{
			Close();
			return false;
		}

		if (!ReadItems())
		{
			Close();
			return false;
		}

		// If the pack has a dictionary, read it
		if (m_HasDict)
		{
			if (!ReadDictionary())
			{
				CH_CORE_WARN("DictionaryPackReader: Pack has dictionary flag but failed to read dictionary");
				// Continue without dictionary - items compressed without dict will fail
			}
		}

		return true;
	}

	void DictionaryPackReader::Close()
	{
		if (m_DCtx)
		{
			ZSTD_freeDCtx((ZSTD_DCtx*)m_DCtx);
			m_DCtx = nullptr;
		}
		if (m_DDict)
		{
			ZSTD_freeDDict((ZSTD_DDict*)m_DDict);
			m_DDict = nullptr;
		}
		if (m_File)
		{
			fclose(m_File);
			m_File = nullptr;
		}
		m_Items.clear();
		m_ItemCount = 0;
		m_HasDict = false;
	}

	bool DictionaryPackReader::ReadHeader()
	{
		PackHeader header;
		if (fread(&header, sizeof(PackHeader), 1, m_File) != 1)
		{
			CH_CORE_ERROR("DictionaryPackReader: Failed to read header");
			return false;
		}

		if (header.magic != PACK_HEADER_MAGIC)
		{
			CH_CORE_ERROR("DictionaryPackReader: Invalid magic number");
			return false;
		}

		if (header.versionMajor != PACK_VERSION_MAJOR || header.versionMinor != PACK_VERSION_MINOR)
		{
			CH_CORE_ERROR("DictionaryPackReader: Version mismatch ({}.{} vs {}.{})", header.versionMajor,
						  header.versionMinor, PACK_VERSION_MAJOR, PACK_VERSION_MINOR);
			return false;
		}

		m_ItemCount = header.itemCount;
		m_DataVersion = header.dataVersion;
		m_PreferSpeed = header.preferSpeed != 0;
		m_HasDict = header._reserved != 0;

		return true;
	}

	bool DictionaryPackReader::ReadItems()
	{
		m_Items.reserve(m_ItemCount);

		for (uint64_t i = 0; i < m_ItemCount; i++)
		{
			PackItemHeader itemHeader;
			if (fread(&itemHeader, sizeof(PackItemHeader), 1, m_File) != 1)
			{
				CH_CORE_ERROR("DictionaryPackReader: Failed to read item header {}", i);
				return false;
			}

			if (itemHeader.dataSize == 0 || itemHeader.pathSize == 0 || itemHeader.dataOffset == 0)
			{
				CH_CORE_ERROR("DictionaryPackReader: Invalid item header {}", i);
				return false;
			}

			char* pathBuf = (char*)malloc(itemHeader.pathSize + 1);
			if (!pathBuf)
			{
				return false;
			}

			if (fread(pathBuf, sizeof(char), itemHeader.pathSize, m_File) != itemHeader.pathSize)
			{
				free(pathBuf);
				CH_CORE_ERROR("DictionaryPackReader: Failed to read item path {}", i);
				return false;
			}
			pathBuf[itemHeader.pathSize] = '\0';

			ItemInfo info;
			info.Path = pathBuf;
			info.DataSize = itemHeader.dataSize;
			info.ZipSize = itemHeader.zipSize;
			info.IsReference = itemHeader.isReference != 0;
			info.DataOffset = itemHeader.dataOffset;

			free(pathBuf);

			// Skip data for non-reference items
			if (!info.IsReference)
			{
				uint64_t skipSize = info.ZipSize > 0 ? info.ZipSize : info.DataSize;
				if (fseek(m_File, (long)skipSize, SEEK_CUR) != 0)
				{
					CH_CORE_ERROR("DictionaryPackReader: Failed to seek past item data {}", i);
					return false;
				}
			}

			m_Items.push_back(std::move(info));
		}

		return true;
	}

	bool DictionaryPackReader::ReadDictionary()
	{
		// The dictionary should be the first item with path "__zstd_dictionary__"
		for (auto& item : m_Items)
		{
			if (item.Path == "__zstd_dictionary__")
			{
				// Read dictionary data
				if (fseek(m_File, (long)item.DataOffset, SEEK_SET) != 0)
				{
					CH_CORE_ERROR("DictionaryPackReader: Failed to seek to dictionary");
					return false;
				}

				std::vector<uint8_t> dictData(item.DataSize);
				if (fread(dictData.data(), 1, item.DataSize, m_File) != item.DataSize)
				{
					CH_CORE_ERROR("DictionaryPackReader: Failed to read dictionary data");
					return false;
				}

				// Create DDict
				m_DDict = ZSTD_createDDict(dictData.data(), dictData.size());
				if (!m_DDict)
				{
					CH_CORE_ERROR("DictionaryPackReader: Failed to create DDict");
					return false;
				}

				// Create per-thread DCtx
				m_DCtx = ZSTD_createDCtx();
				if (!m_DCtx)
				{
					CH_CORE_ERROR("DictionaryPackReader: Failed to create DCtx");
					ZSTD_freeDDict((ZSTD_DDict*)m_DDict);
					m_DDict = nullptr;
					return false;
				}

				CH_CORE_INFO("DictionaryPackReader: Loaded dictionary ({} bytes)", item.DataSize);
				return true;
			}
		}

		CH_CORE_WARN("DictionaryPackReader: No dictionary item found in pack");
		return false;
	}

	bool DictionaryPackReader::GetItemIndex(const char* path, uint64_t& index) const
	{
		std::string searchPath(path);
		for (uint64_t i = 0; i < m_Items.size(); i++)
		{
			if (m_Items[i].Path == searchPath)
			{
				index = i;
				return true;
			}
		}
		return false;
	}

	uint32_t DictionaryPackReader::GetItemDataSize(uint64_t index) const
	{
		return m_Items[index].DataSize;
	}

	uint32_t DictionaryPackReader::GetItemZipSize(uint64_t index) const
	{
		return m_Items[index].ZipSize;
	}

	bool DictionaryPackReader::IsItemReference(uint64_t index) const
	{
		return m_Items[index].IsReference;
	}

	std::string DictionaryPackReader::GetItemPath(uint64_t index) const
	{
		return m_Items[index].Path;
	}

	bool DictionaryPackReader::ReadItemData(uint64_t index, std::vector<uint8_t>& buffer)
	{
		if (index >= m_Items.size())
		{
			return false;
		}

		const ItemInfo& item = m_Items[index];

		// Skip dictionary item - it's not a real asset
		if (item.Path == "__zstd_dictionary__")
		{
			return false;
		}

		// Handle reference items
		if (item.IsReference)
		{
			// Find the referenced item
			for (uint64_t i = 0; i < m_Items.size(); i++)
			{
				if (!m_Items[i].IsReference && m_Items[i].DataOffset == item.DataOffset)
				{
					return ReadItemData(i, buffer);
				}
			}
			return false;
		}

		buffer.resize(item.DataSize);

		if (fseek(m_File, (long)item.DataOffset, SEEK_SET) != 0)
		{
			return false;
		}

		if (item.ZipSize > 0)
		{
			// Compressed data - need to decompress
			std::vector<uint8_t> zipData(item.ZipSize);
			if (fread(zipData.data(), 1, item.ZipSize, m_File) != item.ZipSize)
			{
				return false;
			}

			if (m_PreferSpeed)
			{
				// LZ4 decompression (shouldn't happen in dictionary packs, but handle it)
				CH_CORE_ERROR("DictionaryPackReader: LZ4 decompression not supported in dictionary packs");
				return false;
			}
			else
			{
				// ZSTD decompression with dictionary
				size_t result;
				if (m_DDict)
				{
					result = ZSTD_decompress_usingDDict((ZSTD_DCtx*)m_DCtx, buffer.data(), item.DataSize,
														zipData.data(), item.ZipSize, (ZSTD_DDict*)m_DDict);
				}
				else
				{
					result = ZSTD_decompressDCtx((ZSTD_DCtx*)m_DCtx, buffer.data(), item.DataSize, zipData.data(),
												 item.ZipSize);
				}

				if (ZSTD_isError(result) || result != item.DataSize)
				{
					return false;
				}
			}
		}
		else
		{
			// Uncompressed data
			if (fread(buffer.data(), 1, item.DataSize, m_File) != item.DataSize)
			{
				return false;
			}
		}

		return true;
	}

} // namespace Chained
