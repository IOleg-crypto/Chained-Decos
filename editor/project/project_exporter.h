#ifndef CH_PROJECT_EXPORTER_H
#define CH_PROJECT_EXPORTER_H

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace Chained
{

	struct ExportResult
	{
		bool Success = false;
		bool Cancelled = false;
		std::string Error;
		std::filesystem::path OutDir;
		std::string File;
		uint64_t PackedFileCount = 0;
		uint64_t TotalUncompressedSize = 0;
		uint64_t PackFileSize = 0;
		bool PackSkipped = false;
	};

	/// @brief Callback invoked after each file is packed.
	/// @param packed  Number of files packed so far (1-based).
	/// @param total   Total number of files to pack.
	/// @param file    Virtual item path of the just-packed file.
	using ExportProgressCallback = std::function<void(uint64_t packed, uint64_t total, const std::string& file)>;

	class ProjectExporter
	{
	public:
		/// @brief Export the active project to @p outputDir.
		/// @param outputDir Path to the target folder (will be created if missing).
		/// @param onProgress Optional callback invoked after each file is packed.
		/// @param cancelFlag If non-null, export is aborted between phases when set to true.
		/// @param forceRepack Repack resources.pack even if it is up to date.
		/// @return ExportResult with success flag and optional error message.
		static ExportResult ExportTo(const std::filesystem::path& outputDir,
									 ExportProgressCallback onProgress = nullptr,
									 const std::atomic<bool>* cancelFlag = nullptr, bool forceRepack = false);

	private:
		/// @brief Recursively collect files in @p dir, appending relative paths to @p out.
		static void CollectFiles(const std::filesystem::path& dir, std::vector<std::filesystem::path>& out);

		/// @brief True when @p packPath predates any source file, or covers a different file count.
		static bool IsPackStale(const std::filesystem::path& packPath, const std::vector<std::string>& fileItemPaths,
								uint64_t expectedItemCount);

		/// @brief Copy a single file from @p src to @p dst.
		static bool CopyFile(const std::filesystem::path& src, const std::filesystem::path& dst, std::string& outError);
	};

} // namespace Chained
#endif // CH_PROJECT_EXPORTER_H