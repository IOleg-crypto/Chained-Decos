#include "project_actions.h"
#include "editor.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_events.h"
#include "engine/scene/project_serializer.h"
#include "nfd.h"
#include <filesystem>
#include <format>

namespace CHEngine
{
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
            Application::OnEvent(e);
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
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path buildDir = exePath.parent_path().parent_path();
        std::filesystem::path cwd = std::filesystem::current_path();

        std::vector<std::filesystem::path> searchBases = {cwd, exePath.parent_path(), buildDir, buildDir / "bin", 
                                                          buildDir / "runtime", cwd / "bin", cwd / "build/bin"};

        std::vector<std::string> targetNames = {projectName + ".exe", "ChainedRuntime.exe", "Runtime.exe"};

        for (const auto &base : searchBases)
        {
            for (const auto &name : targetNames)
            {
                if (auto nested = base / configStr / name; std::filesystem::exists(nested))
                    return nested;
                if (auto flat = base / name; std::filesystem::exists(flat))
                    return flat;
            }
        }

        CH_CORE_ERROR("Failed to find runtime executable.");
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

        std::string command = std::format("start \"\" \"{}\" \"{}\"", runtimePath.string(), projectPathStr);
        CH_CORE_INFO("Launching Standalone: {}", command);

        system(command.c_str());
    }
} // namespace CHEngine
