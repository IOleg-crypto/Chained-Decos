#ifndef CH_PROJECT_EXPORTER_H
#define CH_PROJECT_EXPORTER_H

#include <cstdint>
#include <filesystem>
#include <functional>
#include <atomic>
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
};

/// @brief Callback invoked after each file is packed.
/// @param packed  Number of files packed so far (1-based).
/// @param total   Total number of files to pack.
/// @param file    Virtual item path of the just-packed file.
using ExportProgressCallback = std::function<void(uint64_t packed, uint64_t total, const std::string& file)>;

static struct ExportState
{
    bool Open = false;
    bool Success = false;
    std::string Message;
    std::string OutDir;
    std::mutex Mutex;
    bool IsExporting = false;
    // Progress tracking (updated from the background thread under Mutex)
    uint64_t PackedFiles = 0;
    uint64_t TotalFiles = 0;
    std::string CurrentFile;
    // Cancel flag — written by GUI thread, read by worker thread (lock-free)
    std::atomic<bool> CancelRequested{false};
} s_ExportState;

static bool s_ShowEditorSettings = false;

class ProjectExporter
{
public:
    /// @brief Export the active project to @p outputDir.
    /// @param outputDir Path to the target folder (will be created if missing).
    /// @param onProgress Optional callback invoked after each file is packed.
    /// @param cancelFlag If non-null, export is aborted between phases when set to true.
    /// @return ExportResult with success flag and optional error message.
    static ExportResult ExportTo(const std::filesystem::path& outputDir,
                                 ExportProgressCallback onProgress = nullptr,
                                 const std::atomic<bool>* cancelFlag = nullptr);

private:
    /// @brief Recursively collect files in @p dir, appending relative paths to @p out.
    static void CollectFiles(const std::filesystem::path& dir, std::vector<std::filesystem::path>& out);

    /// @brief Copy a single file from @p src to @p dst.
    static bool CopyFile(const std::filesystem::path& src, const std::filesystem::path& dst, std::string& outError);
};

} // namespace Chained
#endif // CH_PROJECT_EXPORTER_H