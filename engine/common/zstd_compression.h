
#ifndef CH_ZSTD_COMPRESSION_H
#define CH_ZSTD_COMPRESSION_H

#include <vector>
#include <string>
#include <zstd.h>
#include "engine/core/log.h"

namespace Chained
{
	class Zstd
	{
	public:
		static std::vector<unsigned char> Compress(const void* src, size_t srcSize, int compressionLevel = 3)
		{
			size_t const maxBounds = ZSTD_compressBound(srcSize);
			std::vector<unsigned char> compressed(maxBounds);

			size_t const cSize = ZSTD_compress(compressed.data(), maxBounds, src, srcSize, compressionLevel);
			if (ZSTD_isError(cSize))
			{
				CH_CORE_ERROR("Zstd: Compression failed: {}", ZSTD_getErrorName(cSize));
				return {};
			}

			compressed.resize(cSize);
			return compressed;
		}

		static std::vector<unsigned char> Decompress(const void* src, size_t srcSize, size_t expectedOriginalSize)
		{
			std::vector<unsigned char> decompressed(expectedOriginalSize);

			size_t const dSize = ZSTD_decompress(decompressed.data(), expectedOriginalSize, src, srcSize);
			if (ZSTD_isError(dSize))
			{
				CH_CORE_ERROR("Zstd: Decompression failed: {}", ZSTD_getErrorName(dSize));
				return {};
			}

			if (dSize != expectedOriginalSize)
			{
				CH_CORE_ERROR("Zstd: Decompression result size mismatch ({} vs expected {})", dSize,
							  expectedOriginalSize);
				return {};
			}

			return decompressed;
		}
	};
} // namespace Chained

#endif // CH_ZSTD_COMPRESSION_H
