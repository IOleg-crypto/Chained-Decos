#include "project_actions.h"
#include "editor.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/project_serializer.h"
#include "nfd.h"
#include <filesystem>
#include <format>
#include "engine/core/log.h"
#include "engine/core/profiler.h"
#include "engine/core/base.h"

namespace CHEngine
{
    void ProjectActions::New()
    {
        // Simple default: close active project to show Project Browser
        Project::SetActive(nullptr);
    }

    void ProjectActions::New(const std::string &name, const std::string &path)
    {
        Project::New();
        auto project = Project::GetActive();
        project->GetConfig().Name = name;
        project->GetConfig().ProjectDirectory = path;

        ProjectSerializer serializer(project);
        serializer.Serialize((std::filesystem::path(path) / (name + ".chproject")).string());
    }

    void ProjectActions::Open()
    {
        nfdchar_t *outPath = NULL;
        nfdu8filteritem_t filterItem[1] = {{"Chained Project", "chproject"}};
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
        if (result == NFD_OKAY)
        {
            Open(outPath);
            NFD_FreePath(outPath);
        }
    }

    void ProjectActions::Open(const std::filesystem::path &path)
    {
        if (Project::Load(path))
        {
            Editor::Get().SetLastProjectPath(path.string());
            ProjectOpenedEvent e(path.string());
            Application::Get().OnEvent(e);
        }
    }

    void ProjectActions::Save()
    {
        auto project = Project::GetActive();
        ProjectSerializer serializer(project);
        serializer.Serialize(
            (project->GetConfig().ProjectDirectory / (project->GetConfig().Name + ".chproject")).string());
    }

    static std::filesystem::path FindRuntimeExecutable(const std::string &projectName, const std::string &configStr)
    {
        CH_PROFILE_FUNCTION();
        
        std::filesystem::path root;
#ifdef PROJECT_ROOT_DIR
        root = PROJECT_ROOT_DIR;
#else
        root = std::filesystem::current_path();
        while (root.has_parent_path() && !std::filesystem::exists(root / "CMakeLists.txt"))
            root = root.parent_path();
#endif

        if (!std::filesystem::exists(root))
        {
            CH_CORE_ERROR("FindRuntimeExecutable: Root path not found: {}", root.string());
            return {};
        }

        CH_CORE_INFO("Searching for runtime in: {}", root.string());

#ifdef CH_PLATFORM_WINDOWS
        std::vector<std::string> targetNames = {projectName + ".exe", "ChainedRuntime.exe", "Runtime.exe"};
#else
        std::vector<std::string> targetNames = {projectName, "ChainedRuntime", "Runtime"};
#endif

        // Declarative search: try each name in order of priority
        for (const auto &name : targetNames)
        {
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
                {
                    // Optimization: ignore large unrelated folders
                    const auto& path = entry.path();
                    auto folderName = path.parent_path().filename().string();
                    
                    if (entry.is_regular_file() && path.filename() == name)
                    {
                        // Check if it's in a 'bin' or 'build' folder to avoid picking up source files or assets
                        std::string pathStr = path.string();
                        if (pathStr.find("build") != std::string::npos || pathStr.find("bin") != std::string::npos)
                        {
                            CH_CORE_INFO("Found runtime candidate: {}", pathStr);
                            return path;
                        }
                    }
                }
            } catch (const std::exception& e) {
                CH_CORE_WARN("Error during runtime search: {}", e.what());
            }
        }

        CH_CORE_ERROR("Failed to find runtime executable among: {}", projectName);
        return {};
    }

    void ProjectActions::LaunchStandalone()
    {
        Save();

        auto project = Project::GetActive();
        if (!project)
            return;

        std::filesystem::path projectFile = project->GetProjectDirectory() / (project->GetConfig().Name + ".chproject");
        std::string projectPathStr = std::filesystem::absolute(projectFile).string();

        if (!std::filesystem::exists(projectPathStr))
        {
            CH_CORE_ERROR("LaunchStandalone: Project file not found: {}", projectPathStr);
            return;
        }

        std::string configStr = (project->GetConfig().BuildConfig == Configuration::Release) ? "Release" : "Debug";
        auto runtimePath = FindRuntimeExecutable(project->GetConfig().Name, configStr);

        if (runtimePath.empty())
            return;

#ifdef CH_PLATFORM_WINDOWS
        std::string command = std::format("start \"\" \"{}\" \"{}\"", runtimePath.string(), projectPathStr);
#else
        std::string command = std::format("\"{}\" \"{}\" &", runtimePath.string(), projectPathStr);
#endif
        CH_CORE_INFO("Launching Standalone: {}", command);

        system(command.c_str());
    }
} // namespace CHEngine
