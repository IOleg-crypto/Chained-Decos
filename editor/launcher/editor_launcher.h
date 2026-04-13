#ifndef CH_EDITOR_LAUNCHER_H
#define CH_EDITOR_LAUNCHER_H

#include "engine/scene/project.h"
#include "engine/scene/scene.h"
#include <memory>
#include <string>
#include <filesystem>

namespace CHEngine {

    class EditorLauncher {
    public:
        /**
         * @brief Launches the standalone runtime for the given project and scene.
         * Resolves project-defined launch profiles or uses default heuristics.
         * @param project The active project.
         * @param editorScene The current editor scene (to sync before launch).
         */
        static void LaunchStandalone(std::shared_ptr<Project> project, std::shared_ptr<Scene> editorScene);

    private:
        static std::filesystem::path FindRuntimeExecutable(const std::string& projectName, const std::string& configStr);
        static std::string ResolveLaunchVariables(std::string str, std::shared_ptr<Project> project);
    };

} // namespace CHEngine
#endif
