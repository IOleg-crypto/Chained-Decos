#include "project_exporter.h"

#include "engine/core/log.h"
#include "engine/core/platform.h"
#include "engine/project/project.h"

#include <algorithm>
#include <atomic>
#include <future>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <pack/writer.hpp>

namespace fs = std::filesystem;

namespace Chained
{

struct ExportCancelledException : public std::exception
{
    const char* what() const noexcept override
    {
        return "Export cancelled by user.";
    }
};

void ProjectExporter::CollectFiles(const fs::path& dir, std::vector<fs::path>& out)
{
    std::error_code ec;
    fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    if (ec)
    {
        CH_CORE_ERROR("ProjectExporter: Cannot iterate '{}': {}", dir.string(), ec.message());
        return;
    }
    for (const auto& entry : it)
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        // Skip 0-byte files (pack reader rejects dataSize == 0)
        std::error_code szEc;
        if (entry.file_size(szEc) == 0)
        {
            continue;
        }

        // Skip build artifacts that shouldn't be packed
        const std::string filename = entry.path().filename().string();
        const std::string ext = entry.path().extension().string();

        // Skip .Up2Date marker files, .pdb, .ilk, .obj, .tlog, .log build intermediates
        if (ext == ".pdb" || ext == ".ilk" || ext == ".obj" || ext == ".tlog" || ext == ".log" || ext == ".Up2Date" ||
            ext == ".FileListAbsolute.txt" || ext == ".lastbuildstate" || ext == ".cache" || ext == ".nupkg" ||
            ext == ".nuget.g.props" || ext == ".nuget.g.targets")
        {
            continue;
        }

        // Skip obj/ and Debug/ and Release/ build output directories
        const auto rel = fs::relative(entry.path(), dir, ec);
        if (!ec)
        {
            std::string relStr = rel.string();
            // Normalize path separators to forward slash for comparison
            std::replace(relStr.begin(), relStr.end(), '\\', '/');
            if (relStr.find("/obj/") != std::string::npos || relStr.find("/Debug/") != std::string::npos ||
                relStr.find("/Release/") != std::string::npos || relStr.find("/x64/") != std::string::npos)
            {
                continue;
            }
            out.push_back(rel);
        }
    }
}

bool ProjectExporter::CopyFile(const fs::path& src, const fs::path& dst, std::string& outError)
{
    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (ec)
    {
        outError = "Failed to create directory '" + dst.parent_path().string() + "': " + ec.message();
        return false;
    }
    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    if (ec)
    {
        outError = "Failed to copy '" + src.string() + "': " + ec.message();
        return false;
    }
    return true;
}

ExportResult ProjectExporter::ExportTo(const fs::path& outputDir, ExportProgressCallback onProgress,
                                       const std::atomic<bool>* cancelFlag)
{
    ExportResult result;
    result.OutDir = outputDir;

    auto CleanupAndCancel = [&](const std::string& phaseLog) -> ExportResult {
        std::error_code cleanEc;
        fs::remove_all(outputDir, cleanEc);
        result.Cancelled = true;
        CH_CORE_INFO("ProjectExporter: Cancelled {}. Cleaned output directory.", phaseLog);
        return result;
    };

    auto project = Project::GetActive();
    if (!project)
    {
        result.Error = "No active project. Please open a project before exporting.";
        CH_CORE_ERROR("ProjectExporter: {}", result.Error);
        return result;
    }

    const auto& cfg = project->GetConfig();
    const fs::path projectDir = project->GetProjectDirectoryForProject();
    const fs::path assetDir = project->GetAssetDirectoryForProject();
    const fs::path exeDir = Platform::GetExecutableDirectory();

    // ── 0. Prepare output directory ──────────────────────────────────────────
    std::error_code ec;
    fs::create_directories(outputDir, ec);
    if (ec)
    {
        result.Error = "Could not create output directory: " + ec.message();
        CH_CORE_ERROR("ProjectExporter: {}", result.Error);
        return result;
    }

    // ── 1. Find .chproject file ──────────────────────────────────────────────
    fs::path chprojectFile;
    for (const auto& entry : fs::directory_iterator(projectDir, ec))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".chproject")
        {
            chprojectFile = entry.path();
            break;
        }
    }
    if (chprojectFile.empty())
    {
        result.Error = "No .chproject file found in '" + projectDir.string() + "'";
        CH_CORE_ERROR("ProjectExporter: {}", result.Error);
        return result;
    }

    // ── 2. Collect files to pack ─────────────────────────────────────────────
    std::vector<std::string> fileItemPaths;

    // .chproject → "project.chproject"
    fileItemPaths.push_back(chprojectFile.generic_string());
    fileItemPaths.push_back("project.chproject");

    // assets/ → "assets/..."
    if (fs::exists(assetDir))
    {
        std::vector<fs::path> assetFiles;
        CollectFiles(assetDir, assetFiles);
        for (const auto& rel : assetFiles)
        {
            fileItemPaths.push_back((assetDir / rel).generic_string());
            fileItemPaths.push_back((fs::path("assets") / rel).generic_string());
        }
    }

    // resources/ → "resources/..."
    fs::path resourcesDir = exeDir / "resources";
    if (fs::exists(resourcesDir))
    {
        std::vector<fs::path> resFiles;
        CollectFiles(resourcesDir, resFiles);
        for (const auto& rel : resFiles)
        {
            fileItemPaths.push_back((resourcesDir / rel).generic_string());
            fileItemPaths.push_back((fs::path("resources") / rel).generic_string());
        }
    }
    else
    {
        CH_CORE_WARN("ProjectExporter: resources/ not found at '{}' — shaders/fonts may be missing from pack.",
                     resourcesDir.string());
    }

    if (fileItemPaths.size() <= 2)
    {
        result.Error = "No files to pack (no assets or resources found).";
        CH_CORE_ERROR("ProjectExporter: {}", result.Error);
        return result;
    }

    if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
    {
        return CleanupAndCancel("before start");
    }

    uint64_t fileCount = fileItemPaths.size() / 2;
    result.PackedFileCount = fileCount;

    // ── 3. Calculate uncompressed size ───────────────────────────────────────
    result.TotalUncompressedSize = 0;
    for (size_t i = 0; i < fileItemPaths.size(); i += 2)
    {
        std::error_code sizeEc;
        auto sz = fs::file_size(fs::path(fileItemPaths[i]), sizeEc);
        if (!sizeEc)
        {
            result.TotalUncompressedSize += sz;
        }
    }

    // ── 4. PARALLEL EXECUTION: Task A (Packing) & Task B (Copying Binaries) ─

    // --- TASK B: Копіювання EXE, DLL та додаткових папок у фоновому потоці ---
    auto copyBinariesTask = std::async(std::launch::async, [&]() -> bool {
        // 1. Copy Executable
        std::vector<fs::path> exeCandidates;
        std::error_code dirEc;
        for (const auto& f : fs::directory_iterator(exeDir, dirEc))
        {
            if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
            {
                return false;
            }
            if (!f.is_regular_file())
            {
                continue;
            }

            std::string fname = f.path().filename().string();
            if (fname.find("Editor") != std::string::npos || fname.find("editor") != std::string::npos ||
                fname.find("test") != std::string::npos)
            {
                continue;
            }
            if (fname.ends_with(".exe") || (fname.find('.') == std::string::npos && !fname.empty()))
            {
                exeCandidates.push_back(f.path());
            }
        }

        if (!exeCandidates.empty())
        {
            const fs::path& srcExe = exeCandidates[0];
            std::string exeName = cfg.Name + srcExe.extension().string();
            fs::path dstExe = outputDir / exeName;
            std::string copyErr;
            if (!CopyFile(srcExe, dstExe, copyErr))
            {
                CH_CORE_ERROR("ProjectExporter: Failed to copy executable: {}", copyErr);
                return false;
            }
        }

        // 2. Copy DLLs
        for (const auto& f : fs::directory_iterator(exeDir, dirEc))
        {
            if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
            {
                return false;
            }
            if (!f.is_regular_file())
            {
                continue;
            }

            const std::string ext = f.path().extension().string();
            if (ext == ".dll" || ext == ".so" || ext == ".dylib")
            {
                std::string copyErr;
                CopyFile(f.path(), outputDir / f.path().filename(), copyErr);
            }
        }

        // 3. Copy Subdirectories
        for (const std::string& subDirName : {"nethost", "dotnet", "scripts"})
        {
            if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
            {
                return false;
            }

            fs::path subSrc = exeDir / subDirName;
            if (fs::exists(subSrc))
            {
                fs::path subDst = outputDir / subDirName;
                std::error_code subEc;
                fs::copy(subSrc, subDst, fs::copy_options::overwrite_existing | fs::copy_options::recursive, subEc);
            }
        }

        // 4. Copy Coral runtime config files to root (needed by ScriptEngine)
        // Coral files are at exeDir/ (next to the exe), not in scripts/{Name}/
        fs::path coralManagedDir = exeDir;
        const char* coralConfigs[] = {"Coral.Managed.runtimeconfig.json", "Coral.Managed.deps.json",
                                      "Coral.Managed.pdb"};
        for (const auto& name : coralConfigs)
        {
            fs::path src = coralManagedDir / name;
            if (fs::exists(src))
            {
                std::string copyErr;
                if (!CopyFile(src, outputDir / name, copyErr))
                {
                    CH_CORE_ERROR("ProjectExporter: Failed to copy Coral file '{}': {}", name, copyErr);
                }
            }
            else
            {
                CH_CORE_WARN("ProjectExporter: Coral file '{}' not found in '{}'", name, coralManagedDir.string());
            }
        }

        // 5. Copy .chproject to output directory
        auto chProjFile = project->GetProjectDirectoryForProject() / (cfg.Name + ".chproject");
        if (!fs::exists(chProjFile))
        {
            // Fallback: find any .chproject in project dir
            for (const auto& entry : fs::directory_iterator(project->GetProjectDirectoryForProject()))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".chproject")
                {
                    chProjFile = entry.path();
                    break;
                }
            }
        }
        if (fs::exists(chProjFile))
        {
            std::string copyErr;
            if (!CopyFile(chProjFile, outputDir / (cfg.Name + ".chproject"), copyErr))
            {
                CH_CORE_ERROR("ProjectExporter: Failed to copy .chproject: {}", copyErr);
            }
        }

        return true;
    });

    // --- TASK A: Пакування ресурсів у головному потоці ---
    const fs::path packPath = outputDir / "resources.pack";
    bool packSuccess = false;

    try
    {
        std::vector<const char*> rawPaths;
        rawPaths.reserve(fileItemPaths.size());
        for (const auto& s : fileItemPaths)
        {
            rawPaths.push_back(s.c_str());
        }

        const auto& exp = project->GetConfig().Export;

        bool preferSpeed = (exp.Mode == PackMode::Fast);
        float threshold = (exp.Mode == PackMode::Raw) ? 0.99f : exp.ZipThreshold;

        struct PackCtx
        {
            const std::vector<std::string>& fileItemPaths;
            uint64_t total;
            ExportProgressCallback& cb;
            const std::atomic<bool>* cancelFlag;
        };
        PackCtx ctx{fileItemPaths, fileCount, onProgress, cancelFlag};

        OnPackFile cCallback = [](uint64_t itemIndex, void* arg) {
            auto* ctx = static_cast<PackCtx*>(arg);

            if (ctx->cancelFlag && ctx->cancelFlag->load(std::memory_order_relaxed))
            {
                throw ExportCancelledException();
            }

            if (ctx->cb)
            {
                uint64_t packed = itemIndex + 1;
                const std::string& itemPath = ctx->fileItemPaths[itemIndex * 2 + 1];
                ctx->cb(packed, ctx->total, itemPath);
            }
        };

        pack::Writer::pack(packPath, fileCount, rawPaths.data(), exp.DataVersion, threshold, preferSpeed, false,
                           cCallback, &ctx);

        packSuccess = true;
    } catch (const ExportCancelledException&)
    {
        copyBinariesTask.wait(); // Чекаємо фоновий потік перед очищенням
        return CleanupAndCancel("during packing process");
    } catch (const pack::Error& err)
    {
        copyBinariesTask.wait();
        CleanupAndCancel("due to pack error");
        result.Error = "Pack failed: " + std::string(err.what());
        CH_CORE_ERROR("ProjectExporter: {}", result.Error);
        return result;
    }

    // Чекаємо завершення копіювання файлів
    bool copySuccess = copyBinariesTask.get();

    // Перевірка на скасування після обох завдань
    if (cancelFlag && cancelFlag->load(std::memory_order_relaxed))
    {
        return CleanupAndCancel("after execution tasks");
    }

    if (!copySuccess || !packSuccess)
    {
        return CleanupAndCancel("due to task failure");
    }

    // ── 5. Get Pack File Size ────────────────────────────────────────────────
    std::error_code sizeEc;
    result.PackFileSize = static_cast<uint64_t>(fs::file_size(packPath, sizeEc));
    if (sizeEc)
    {
        result.PackFileSize = 0;
    }

    result.Success = true;
    CH_CORE_INFO("ProjectExporter: Export complete → '{}' ({} packed, {} MB pack)", outputDir.string(),
                 result.PackedFileCount, result.PackFileSize / (1024 * 1024));
    return result;
}

} // namespace Chained