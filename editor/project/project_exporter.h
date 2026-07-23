#ifndef CH_PROJECT_EXPORTER_H
#define CH_PROJECT_EXPORTER_H

#include <cstdint>
#include <filesystem>
#include <string>

namespace Chained
{

struct ExportResult
{
    bool Success = false;
    std::string Error;
    std::filesystem::path OutDir;
    uint64_t PackedFileCount = 0;
    uint64_t TotalUncompressedSize = 0;
    uint64_t PackFileSize = 0;
};

class ProjectExporter
{
public:
    /// @brief Export the active project to @p outputDir.
    /// @param outputDir Path to the target folder (will be created if missing).
    /// @return ExportResult with success flag and optional error message.
    static ExportResult ExportTo(const std::filesystem::path& outputDir);

private:
    /// @brief Recursively collect files in @p dir, appending relative paths to @p out.
    static void CollectFiles(const std::filesystem::path& dir, std::vector<std::filesystem::path>& out);

    /// @brief Copy a single file from @p src to @p dst.
    static bool CopyFile(const std::filesystem::path& src, const std::filesystem::path& dst, std::string& outError);
};

} // namespace Chained
#endif // CH_PROJECT_EXPORTER_H