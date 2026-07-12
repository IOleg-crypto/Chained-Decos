#ifndef CH_PROJECT_EXPORTER_H
#define CH_PROJECT_EXPORTER_H

#include <filesystem>
#include <string>

namespace Chained
{

/// Result of a project export operation.
struct ExportResult
{
    bool        Success    = false;
    std::string Error;           // Non-empty when Success == false
    std::filesystem::path OutDir; // Target directory that was written to
};

/// @brief Packages the current project into a standalone, redistributable directory.
///
/// The exported folder contains:
///   - <ProjectName>.exe   – the game runtime binary
///   - <ProjectName>.chproject
///   - assets/             – all game assets copied from the project's asset dir
///   - resources/          – engine shaders, fonts, icons
///   - Chained.Managed.dll – C# scripting runtime
///   - <Scripts>.dll       – compiled C# game scripts
///
/// Usage:
///   auto result = ProjectExporter::ExportTo("/path/to/output");
class ProjectExporter
{
public:
    /// @brief Export the active project to @p outputDir.
    /// @param outputDir Path to the target folder (will be created if missing).
    /// @return ExportResult with success flag and optional error message.
    static ExportResult ExportTo(const std::filesystem::path& outputDir);

private:
    static bool CopyDirRecursive(const std::filesystem::path& src,
                                 const std::filesystem::path& dst,
                                 std::string& outError);
};

} // namespace Chained
#endif // CH_PROJECT_EXPORTER_H