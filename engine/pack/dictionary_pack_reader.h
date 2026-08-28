#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Chained
{
	class DictionaryPackReader
	{
	public:
		DictionaryPackReader() = default;
		~DictionaryPackReader();

		DictionaryPackReader(const DictionaryPackReader&) = delete;
		DictionaryPackReader& operator=(const DictionaryPackReader&) = delete;
		DictionaryPackReader(DictionaryPackReader&& other) noexcept;
		DictionaryPackReader& operator=(DictionaryPackReader&& other) noexcept;

		bool Open(const std::filesystem::path& packPath);
		void Close();

		bool IsOpen() const
		{
			return m_File != nullptr;
		}
		bool HasDictionary() const
		{
			return m_DDict != nullptr;
		}
		uint64_t GetItemCount() const
		{
			return m_ItemCount;
		}

		bool GetItemIndex(const char* path, uint64_t& index) const;
		uint32_t GetItemDataSize(uint64_t index) const;
		uint32_t GetItemZipSize(uint64_t index) const;
		bool IsItemReference(uint64_t index) const;
		std::string GetItemPath(uint64_t index) const;

		bool ReadItemData(uint64_t index, std::vector<uint8_t>& buffer);

	private:
		struct ItemInfo
		{
			std::string Path;
			uint32_t DataSize = 0;
			uint32_t ZipSize = 0;
			bool IsReference = false;
			uint64_t DataOffset = 0;
		};

		bool ReadHeader();
		bool ReadItems();
		bool ReadDictionary();

		FILE* m_File = nullptr;
		uint64_t m_ItemCount = 0;
		uint32_t m_DataVersion = 0;
		bool m_PreferSpeed = false;
		bool m_HasDict = false;

		void* m_DDict = nullptr; // ZSTD_DDict*
		void* m_DCtx = nullptr;	 // ZSTD_DCtx*

		std::vector<ItemInfo> m_Items;
	};
} // namespace Chained
