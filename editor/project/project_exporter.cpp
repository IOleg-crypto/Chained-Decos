#include "project_exporter.h"

#include "engine/core/log.h"
#include "engine/project/project.h"

#include <filesystem>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace Chained
{

bool ProjectExporter::CopyDirRecursive(const fs::path& src, const fs::path& dst, std::string& outError)
{
    std::error_code ec;
    fs::create_directories(dst, ec);
    if (ec)
    {
        outError = "Failed to create directory '" + dst.string() + "': " + ec.message();
        return false;
    }

    for (const auto& entry : fs::recursive_directory_iterator(src, ec))
    {
        if (ec) break;
        const fs::path rel  = fs::relative(entry.path(), src, ec);
        const fs::path dest = dst / rel;
        if (entry.is_directory())
        {
            fs::create_directories(dest, ec);
        }
        else
        {
            fs::create_directories(dest.parent_path(), ec);
            fs::copy_file(entry.path(), dest, fs::copy_options::overwrite_existing, ec);
        }
        if (ec)
        {
            outError = "Copy error: " + ec.message() + " (" + entry.path().string() + ")";
            return false;
        }
    }
    if (ec)
    {
        outError = "Directory iteration error: " + ec.message();
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

    const auto& cfg  = project->GetConfig();
    std::error_code  ec;

    // ── 0. Prepare output directory ──────────────────────────────────────────
    fs::create_directories(outputDir, ec);
    if (ec)
    {
        result.Error = "Could not create output directory: " + ec.message();
        CH_CORE_ERROR("ProjectExporter: {}", result.Error);
        return result;
    }

    // The game executable and all engine binaries live next to *this* editor executable.
    // __argv[0] / fs::current_path are not reliable across platforms, so we derive the
    // executable directory from the project directory (which is always a sibling of the
    // executable in our build layout).
    const fs::path projectDir = project->GetProjectDirectory();

    // Try to locate the bin directory relative to the project:
    // Layout: <repo>/build/<preset>/bin/<Config>/  (editor + game exe live here)
    // We walk up until we find a folder called "bin" or fall back to current_path().
    fs::path exeDir = fs::current_path();
    {
        fs::path search = projectDir;
        for (int i = 0; i < 6; ++i)
        {
            fs::path candidate = search / "bin";
            if (fs::exists(candidate))
            {
                // The real binaries are one level deeper (Debug / Release).
                // Pick whichever subfolder contains the game exe.
                for (const auto& sub : fs::directory_iterator(candidate, ec))
                {
                    if (!sub.is_directory()) continue;
                    // Look for the runtime exe (name = project name, no "editor" suffix)
                    for (const auto& f : fs::directory_iterator(sub.path(), ec))
                    {
                        std::string fname = f.path().filename().string();
                        // Match e.g. "ChainedDecos.exe" (not "ChainedDecosEditor.exe" or tests)
                        if (fname.find("Editor") == std::string::npos &&
                            fname.find("editor") == std::string::npos &&
                            fname.find("test")   == std::string::npos &&
                            (fname.ends_with(".exe") || fname.find('.') == std::string::npos))
                        {
                            exeDir = sub.path();
                            goto foundExeDir;
                        }
                    }
                }
            }
            search = search.parent_path();
        }
        foundExeDir:;
    }

    CH_CORE_INFO("ProjectExporter: resolved exe dir = '{}'", exeDir.string());

    // ── 1. Copy game executable ───────────────────────────────────────────────
    {
        // Game exe name = project name (without "Editor")
        std::vector<fs::path> candidates;
        for (const auto& f : fs::directory_iterator(exeDir, ec))
        {
            std::string fname = f.path().filename().string();
            if (fname.find("Editor") == std::string::npos &&
                fname.find("editor") == std::string::npos &&
                fname.find("test")   == std::string::npos &&
                (fname.ends_with(".exe") || (!fname.empty() && fname.find('.') == std::string::npos)))
            {
                candidates.push_back(f.path());
            }
        }

        if (candidates.empty())
        {
            result.Error = "Could not find game executable in '" + exeDir.string() + "'. "
                           "Please build the project first.";
            CH_CORE_ERROR("ProjectExporter: {}", result.Error);
            return result;
        }

        // Use the first candidate (usually the only one)
        const fs::path& srcExe = candidates[0];
        // Rename to the project name in the output folder
        std::string    gameExeName = cfg.Name + srcExe.extension().string();
        fs::path       dstExe      = outputDir / gameExeName;
        fs::copy_file(srcExe, dstExe, fs::copy_options::overwrite_existing, ec);
        if (ec)
        {
            result.Error = "Failed to copy executable '" + srcExe.string() + "': " + ec.message();
            CH_CORE_ERROR("ProjectExporter: {}", result.Error);
            return result;
        }
        CH_CORE_INFO("ProjectExporter: copied exe '{}' -> '{}'", srcExe.string(), dstExe.string());
    }

    // ── 2. Copy all *.dll next to the exe (scripting + mono + dotnet) ────────
    for (const auto& f : fs::directory_iterator(exeDir, ec))
    {
        if (!f.is_regular_file()) continue;
        const std::string ext = f.path().extension().string();
        if (ext == ".dll" || ext == ".so" || ext == ".dylib" || ext == ".json" || ext == ".pdb")
        {
            fs::path dst = outputDir / f.path().filename();
            fs::copy_file(f.path(), dst, fs::copy_options::overwrite_existing, ec);
            if (ec) { ec.clear(); } // non-fatal: best-effort
        }
    }

    // Some runtimes also place .dll in subdirs (e.g. nethost/)
    for (const std::string& subDirName : { "nethost", "dotnet", "scripts" })
    {
        fs::path subSrc = exeDir / subDirName;
        if (fs::exists(subSrc))
        {
            fs::path subDst = outputDir / subDirName;
            CopyDirRecursive(subSrc, subDst, result.Error); // non-fatal
            result.Error.clear();
        }
    }

    // ── 3. Copy .chproject file ───────────────────────────────────────────────
    {
        // Look for the .chproject next to the exe first, then in the project dir
        fs::path srcProject;
        for (const auto& f : fs::directory_iterator(exeDir, ec))
        {
            if (f.path().extension() == ".chproject") { srcProject = f.path(); break; }
        }
        if (srcProject.empty())
        {
            // Fall back: find in the game/chaineddecos directory
            for (const auto& f : fs::directory_iterator(projectDir, ec))
            {
                if (f.path().extension() == ".chproject") { srcProject = f.path(); break; }
            }
        }

        if (!srcProject.empty())
        {
            fs::path dst = outputDir / srcProject.filename();
            fs::copy_file(srcProject, dst, fs::copy_options::overwrite_existing, ec);
            if (ec) { CH_CORE_WARN("ProjectExporter: could not copy .chproject: {}", ec.message()); ec.clear(); }
        }
        else
        {
            CH_CORE_WARN("ProjectExporter: .chproject not found — export will proceed without it.");
        }
    }

    // ── 4. Copy assets/ ────────────────────────────────────────────────────
    {
        fs::path assetSrc = project->GetAssetDirectory();
        if (fs::exists(assetSrc))
        {
            fs::path assetDst = outputDir / "assets";
            if (!CopyDirRecursive(assetSrc, assetDst, result.Error))
            {
                CH_CORE_ERROR("ProjectExporter: {}", result.Error);
                return result;
            }
            CH_CORE_INFO("ProjectExporter: assets copied.");
        }
    }

    // ── 5. Copy resources/ (shaders, fonts, icons) ───────────────────────────
    {
        // resources/ lives at the same level as assets/ in the exe dir
        fs::path resSrc = exeDir / "resources";
        if (fs::exists(resSrc))
        {
            fs::path resDst = outputDir / "resources";
            if (!CopyDirRecursive(resSrc, resDst, result.Error))
            {
                CH_CORE_ERROR("ProjectExporter: {}", result.Error);
                return result;
            }
            CH_CORE_INFO("ProjectExporter: resources copied.");
        }
        else
        {
            CH_CORE_WARN("ProjectExporter: resources/ not found at '{}' — shaders may be missing.", resSrc.string());
        }
    }

    result.Success = true;
    CH_CORE_INFO("ProjectExporter: Export complete → '{}'", outputDir.string());
    return result;
}

} // namespace Chained
