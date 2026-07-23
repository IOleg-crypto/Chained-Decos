#include "project_exporter.h"

#include "engine/core/log.h"
#include "engine/core/platform.h"
#include "engine/project/project.h"

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
#include <pack/writer.hpp>

namespace fs = std::filesystem;

namespace Chained
{

void ProjectExporter::CollectFiles(const fs::path& dir, std::vector<fs::path>& out)
{
    std::error_code ec;
    for (const auto& entry : fs::recursive_directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        std::error_code recEc;
        out.push_back(fs::relative(entry.path(), dir, recEc));
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

ExportResult ProjectExporter::ExportTo(const fs::path& outputDir)
{
    ExportResult result;
    result.OutDir = outputDir;

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
    //    Pack structure: project.chproject, assets/*, resources/*
    std::vector<std::string> fileItemPaths;

    // .chproject → "project.chproject"
    {
        std::string file = chprojectFile.generic_string();
        std::string item = "project.chproject";
        fileItemPaths.push_back(file);
        fileItemPaths.push_back(item);
    }

    // assets/ → "assets/..."
    if (fs::exists(assetDir))
    {
        std::vector<fs::path> assetFiles;
        CollectFiles(assetDir, assetFiles);
        for (const auto& rel : assetFiles)
        {
            std::string file = (assetDir / rel).generic_string();
            std::string item = (fs::path("assets") / rel).generic_string();
            fileItemPaths.push_back(file);
            fileItemPaths.push_back(item);
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
            std::string file = (resourcesDir / rel).generic_string();
            std::string item = (fs::path("resources") / rel).generic_string();
            fileItemPaths.push_back(file);
            fileItemPaths.push_back(item);
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

    uint64_t fileCount = fileItemPaths.size() / 2;
    result.PackedFileCount = fileCount;

    // ── 3. Pack into resources.pack ──────────────────────────────────────────
    {
        const fs::path packPath = outputDir / "resources.pack";
        std::vector<const char*> rawPaths;
        rawPaths.reserve(fileItemPaths.size());
        for (const auto& s : fileItemPaths)
        {
            rawPaths.push_back(s.c_str());
        }
        try
        {
            const auto& exp = project->GetConfig().Export;
            pack::Writer::pack(packPath, fileCount, rawPaths.data(), exp.DataVersion, exp.ZipThreshold, exp.PreferSpeed,
                               false);
        } catch (const pack::Error& err)
        {
            result.Error = "Pack failed: " + std::string(err.what());
            CH_CORE_ERROR("ProjectExporter: {}", result.Error);
            return result;
        }

        std::error_code sizeEc;
        result.PackFileSize = static_cast<std::uint64_t>(fs::file_size(packPath, sizeEc));
        if (sizeEc)
        {
            result.PackFileSize = 0;
        }

        CH_CORE_INFO("ProjectExporter: packed {} files → '{}' ({} bytes)", fileCount, packPath.string(),
                     result.PackFileSize);
    }

    // ── 4. Calculate total uncompressed size ─────────────────────────────────
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

    // ── 5. Copy executable ───────────────────────────────────────────────────
    {
        std::vector<fs::path> exeCandidates;
        for (const auto& f : fs::directory_iterator(exeDir, ec))
        {
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
                result.Error = "Failed to copy executable: " + copyErr;
                CH_CORE_ERROR("ProjectExporter: {}", result.Error);
                return result;
            }
            CH_CORE_INFO("ProjectExporter: copied exe '{}' → '{}'", srcExe.string(), dstExe.string());
        }
        else
        {
            CH_CORE_WARN("ProjectExporter: game executable not found in '{}' — skipping exe copy.", exeDir.string());
        }
    }

    // ── 6. Copy DLLs and shared libraries ────────────────────────────────────
    for (const auto& f : fs::directory_iterator(exeDir, ec))
    {
        if (!f.is_regular_file())
        {
            continue;
        }
        const std::string ext = f.path().extension().string();
        if (ext == ".dll" || ext == ".so" || ext == ".dylib")
        {
            std::string copyErr;
            CopyFile(f.path(), outputDir / f.path().filename(), copyErr); // non-fatal
        }
    }

    // ── 7. Copy subdirectories (nethost, dotnet, scripts) ────────────────────
    for (const std::string& subDirName : {"nethost", "dotnet", "scripts"})
    {
        fs::path subSrc = exeDir / subDirName;
        if (fs::exists(subSrc))
        {
            fs::path subDst = outputDir / subDirName;
            std::error_code subEc;
            fs::copy(subSrc, subDst, fs::copy_options::overwrite_existing | fs::copy_options::recursive, subEc);
        }
    }

    result.Success = true;
    CH_CORE_INFO("ProjectExporter: Export complete → '{}' ({} packed, {} MB pack)", outputDir.string(),
                 result.PackedFileCount, result.PackFileSize / (1024 * 1024));
    return result;
}

} // namespace Chained
