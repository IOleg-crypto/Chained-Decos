extern "C" {
#include "pack/writer.h"
#include "pack/common.h"
}

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <zstd.h>
#include <zdict.h>

static void printUsage()
{
	std::cout << "Usage: chpacker [options] <pack-path> [file-path-1 item-path-1 ...]\n"
			  << "\n"
			  << "Options:\n"
			  << "  -z <zipThreshold>  Compression threshold (0-100%, default: 10)\n"
			  << "  -v <dataVersion>   Pack data version (default: 0)\n"
			  << "  -s                 Prefer speed (LZ4) over compression size (ZSTD)\n"
			  << "  --dict             Use ZSTD dictionary compression (best for mixed assets)\n"
			  << "  -m <manifest-file> Read file/item pairs from a manifest file (tab-separated)\n"
			  << "  -h, --help         Show this help message\n";
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printUsage();
		return EXIT_FAILURE;
	}

	float zipThreshold = 0.1f;
	uint32_t dataVersion = 0;
	bool preferSpeed = false;
	bool useDictionary = false;
	std::string manifestFile;
	std::string packPath;
	int argOffset = 1;

	while (argOffset < argc)
	{
		std::string arg = argv[argOffset];
		if (arg == "-z" && argOffset + 1 < argc)
		{
			int zipPercents = std::atoi(argv[++argOffset]);
			if (zipPercents < 0 || zipPercents > 100)
			{
				std::cerr << "Error: zip threshold must be between 0 and 100.\n";
				return EXIT_FAILURE;
			}
			zipThreshold = static_cast<float>(zipPercents) * 0.01f;
			argOffset++;
		}
		else if (arg == "-v" && argOffset + 1 < argc)
		{
			long long version = std::atoll(argv[++argOffset]);
			if (version < 0 || version > UINT32_MAX)
			{
				std::cerr << "Error: data version out of range.\n";
				return EXIT_FAILURE;
			}
			dataVersion = static_cast<uint32_t>(version);
			argOffset++;
		}
		else if (arg == "-s")
		{
			preferSpeed = true;
			argOffset++;
		}
		else if (arg == "--dict")
		{
			useDictionary = true;
			argOffset++;
		}
		else if (arg == "-m" && argOffset + 1 < argc)
		{
			manifestFile = argv[++argOffset];
			argOffset++;
		}
		else if (arg == "-h" || arg == "--help")
		{
			printUsage();
			return EXIT_SUCCESS;
		}
		else if (!arg.empty() && arg[0] == '-')
		{
			std::cerr << "Error: unknown option: " << arg << "\n";
			printUsage();
			return EXIT_FAILURE;
		}
		else
		{
			// First non-flag argument is the output pack file path
			if (packPath.empty())
			{
				packPath = argv[argOffset++];
			}
			else
			{
				break;
			}
		}
	}

	if (packPath.empty())
	{
		std::cerr << "Error: Output pack file path is required.\n";
		printUsage();
		return EXIT_FAILURE;
	}

	std::vector<std::string> fileList;

	if (!manifestFile.empty())
	{
		std::ifstream mf(manifestFile);
		if (!mf.is_open())
		{
			std::cerr << "Error: Cannot open manifest file: " << manifestFile << "\n";
			return EXIT_FAILURE;
		}
		std::string line;
		while (std::getline(mf, line))
		{
			// Trim carriage return if present
			if (!line.empty() && line.back() == '\r')
			{
				line.pop_back();
			}

			if (line.empty() || line[0] == '#')
			{
				continue;
			}

			size_t tabPos = line.find('\t');
			if (tabPos != std::string::npos)
			{
				std::string filePath = line.substr(0, tabPos);
				std::string itemPath = line.substr(tabPos + 1);
				if (!filePath.empty() && !itemPath.empty())
				{
					fileList.push_back(filePath);
					fileList.push_back(itemPath);
				}
			}
			else
			{
				std::cerr << "Warning: Skipping line without tab delimiter: " << line << "\n";
			}
		}
	}
	else
	{
		while (argOffset < argc)
		{
			fileList.emplace_back(argv[argOffset++]);
		}
	}

	if (fileList.empty() || fileList.size() % 2 != 0)
	{
		std::cerr << "Error: Invalid items count (" << fileList.size()
				  << "). Expected pairs of <file-path> <item-path>.\n";
		return EXIT_FAILURE;
	}

	size_t fileCount = fileList.size() / 2;
	std::vector<const char*> rawPointers;
	rawPointers.reserve(fileList.size());
	for (const auto& entry : fileList)
	{
		rawPointers.push_back(entry.c_str());
	}

	std::cout << "Packaging " << fileCount << " file(s) into " << packPath << "...\n";

	if (useDictionary)
	{
		// Dictionary mode: read all files, train dictionary, compress with dictionary
		struct FileData
		{
			std::string itemPath;
			std::vector<uint8_t> data;
		};

		std::vector<FileData> files(fileCount);
		uint64_t totalRawSize = 0;

		for (size_t i = 0; i < fileCount; i++)
		{
			files[i].itemPath = fileList[i * 2 + 1];
			std::string filePath = fileList[i * 2];

			FILE* f = fopen(filePath.c_str(), "rb");
			if (!f)
			{
				std::cerr << "Error: Cannot open file: " << filePath << "\n";
				return EXIT_FAILURE;
			}

			fseek(f, 0, SEEK_END);
			long size = ftell(f);
			fseek(f, 0, SEEK_SET);

			if (size > 0)
			{
				files[i].data.resize(size);
				fread(files[i].data.data(), 1, size, f);
				totalRawSize += size;
			}
			fclose(f);
		}

		// Train dictionary
		std::cout << "Training ZSTD dictionary...\n";

		std::vector<uint8_t> samplesBuffer;
		std::vector<size_t> sampleSizes;
		for (auto& f : files)
		{
			if (!f.data.empty())
			{
				samplesBuffer.insert(samplesBuffer.end(), f.data.begin(), f.data.end());
				sampleSizes.push_back(f.data.size());
			}
		}

		size_t dictCapacity = std::max<size_t>(100 * 1024, totalRawSize / 100);
		dictCapacity = std::min(dictCapacity, (size_t)1 << 20);

		std::vector<uint8_t> dictBuffer(dictCapacity);
		size_t dictSize = ZDICT_trainFromBuffer(dictBuffer.data(), dictCapacity, samplesBuffer.data(),
												sampleSizes.data(), (unsigned)sampleSizes.size());

		if (ZDICT_isError(dictSize))
		{
			std::cerr << "Warning: Dictionary training failed: " << ZDICT_getErrorName(dictSize) << "\n";
			std::cerr << "Falling back to standard ZSTD compression.\n";
			dictSize = 0;
		}
		else
		{
			std::cout << "Dictionary trained (" << dictSize << " bytes)\n";
		}

		// Create CDict
		ZSTD_CDict* cdict = nullptr;
		ZSTD_CCtx* cctx = ZSTD_createCCtx();

		if (dictSize > 0)
		{
			cdict = ZSTD_createCDict(dictBuffer.data(), dictSize, ZSTD_maxCLevel());
			if (!cdict)
			{
				std::cerr << "Error: Failed to create CDict\n";
				ZSTD_freeCCtx(cctx);
				return EXIT_FAILURE;
			}
		}

		// Compress each file
		std::cout << "Compressing files...\n";

		std::vector<std::vector<uint8_t>> compressedData(fileCount);
		std::vector<uint32_t> compressedSizes(fileCount, 0);

		for (size_t i = 0; i < fileCount; i++)
		{
			if (files[i].data.empty())
			{
				continue;
			}

			size_t bound = ZSTD_compressBound(files[i].data.size());
			compressedData[i].resize(bound);

			size_t result;
			if (cdict)
			{
				result = ZSTD_compress_usingCDict(cctx, compressedData[i].data(), bound, files[i].data.data(),
												  files[i].data.size(), cdict);
			}
			else
			{
				result = ZSTD_compressCCtx(cctx, compressedData[i].data(), bound, files[i].data.data(),
										   files[i].data.size(), ZSTD_maxCLevel());
			}

			if (ZSTD_isError(result))
			{
				compressedSizes[i] = 0;
				compressedData[i].clear();
			}
			else
			{
				compressedSizes[i] = (uint32_t)result;
				compressedData[i].resize(result);
			}

			int progress = (int)(((float)(i + 1) / (float)fileCount) * 100.0f);
			if (compressedSizes[i] > 0)
			{
				std::cout << "[" << progress << "%] " << files[i].itemPath << " (" << compressedSizes[i] << "/"
						  << files[i].data.size() << " bytes)\n";
			}
			else
			{
				std::cout << "[" << progress << "%] " << files[i].itemPath << " (stored, " << files[i].data.size()
						  << " bytes)\n";
			}
		}

		// Write pack file
		std::cout << "Writing pack file...\n";

		FILE* pf = fopen(packPath.c_str(), "wb");
		if (!pf)
		{
			std::cerr << "Error: Cannot create file: " << packPath << "\n";
			if (cdict)
			{
				ZSTD_freeCDict(cdict);
			}
			ZSTD_freeCCtx(cctx);
			return EXIT_FAILURE;
		}

		// Write header
		PackHeader header;
		header.magic = PACK_HEADER_MAGIC;
		header.versionMajor = PACK_VERSION_MAJOR;
		header.versionMinor = PACK_VERSION_MINOR;
		header.versionPatch = PACK_VERSION_PATCH;
		header.isBigEndian = !PACK_LITTLE_ENDIAN;
		header.itemCount = fileCount + (dictSize > 0 ? 1 : 0);
		header.dataVersion = dataVersion;
		header.preferSpeed = 0;
		header._reserved = dictSize > 0 ? 1 : 0;

		fwrite(&header, sizeof(PackHeader), 1, pf);

		uint64_t fileOffset = sizeof(PackHeader);

		// Write dictionary as first item
		if (dictSize > 0)
		{
			const char* dictPath = "__zstd_dictionary__";
			uint8_t pathSize = (uint8_t)strlen(dictPath);

			PackItemHeader itemHeader;
			itemHeader.dataSize = (uint32_t)dictSize;
			itemHeader.zipSize = 0;
			itemHeader.pathSize = pathSize;
			itemHeader.isReference = 0;
			itemHeader.dataOffset = fileOffset + sizeof(PackItemHeader) + pathSize;

			fwrite(&itemHeader, sizeof(PackItemHeader), 1, pf);
			fwrite(dictPath, sizeof(char), pathSize, pf);
			fwrite(dictBuffer.data(), sizeof(uint8_t), dictSize, pf);

			fileOffset += sizeof(PackItemHeader) + pathSize + dictSize;
		}

		// Write items
		for (size_t i = 0; i < fileCount; i++)
		{
			uint8_t pathSize = (uint8_t)files[i].itemPath.size();
			uint32_t dataSize = (uint32_t)files[i].data.size();
			uint32_t zipSize = compressedSizes[i];

			PackItemHeader itemHeader;
			itemHeader.dataSize = dataSize;
			itemHeader.zipSize = zipSize;
			itemHeader.pathSize = pathSize;
			itemHeader.isReference = 0;
			itemHeader.dataOffset = fileOffset + sizeof(PackItemHeader) + pathSize;

			fwrite(&itemHeader, sizeof(PackItemHeader), 1, pf);
			fwrite(files[i].itemPath.c_str(), sizeof(char), pathSize, pf);

			fileOffset += sizeof(PackItemHeader) + pathSize;

			if (zipSize > 0)
			{
				fwrite(compressedData[i].data(), sizeof(uint8_t), zipSize, pf);
				fileOffset += zipSize;
			}
			else if (dataSize > 0)
			{
				fwrite(files[i].data.data(), sizeof(uint8_t), dataSize, pf);
				fileOffset += dataSize;
			}
		}

		fclose(pf);

		if (cdict)
		{
			ZSTD_freeCDict(cdict);
		}
		ZSTD_freeCCtx(cctx);

		uint64_t packSize = 0;
		FILE* checkFile = fopen(packPath.c_str(), "rb");
		if (checkFile)
		{
			fseek(checkFile, 0, SEEK_END);
			packSize = ftell(checkFile);
			fclose(checkFile);
		}

		int compression = (int)((1.0 - (double)packSize / (double)totalRawSize) * 100.0);
		std::cout << "Packed " << fileCount << " files. (" << packSize << "/" << totalRawSize << " bytes, "
				  << compression << "% saved)\n";
	}
	else
	{
		// Standard mode
		PackResult result = packFiles(packPath.c_str(), static_cast<uint64_t>(fileCount), rawPointers.data(),
									  dataVersion, zipThreshold, preferSpeed, true, nullptr, nullptr);

		if (result != SUCCESS_PACK_RESULT)
		{
			std::cerr << "\nError: " << packResultToString(result) << "\n";
			return EXIT_FAILURE;
		}
	}

	std::cout << "Packaging completed successfully.\n";
	return EXIT_SUCCESS;
}
