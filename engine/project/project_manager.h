#ifndef CH_PROJECT_MANAGER_H
#define CH_PROJECT_MANAGER_H

#include "engine/project/project.h"
#include <memory>
#include <filesystem>


namespace CHEngine
{
    class ProjectManager
    {
    public:
        static void Init();
        static void Shutdown();
        static ProjectManager& Get();

        // IFileSystem implementation
        std::filesystem::path ResolvePath(const std::string& virtualPath) const { return GetAbsolutePath(virtualPath); }

        // Helper proxies for the active project (now public for modular access)
        std::filesystem::path GetAssetDirectory() const;
        std::filesystem::path GetProjectDirectory() const;
        std::filesystem::path GetAssetPath(const std::filesystem::path& relative) const;

        std::string GetRelativePath(const std::filesystem::path& path) const;
        std::filesystem::path GetAbsolutePath(const std::filesystem::path& path) const;

        std::shared_ptr<Project> GetActive() const { return m_ActiveProject; }

    private:
        ProjectManager();
        virtual ~ProjectManager() = default;

        friend class Project;
        friend class AssetManager;

        void SetActive(std::shared_ptr<Project> project);

        void SetEngineRoot(const std::filesystem::path& path);
        const std::filesystem::path& GetEngineRoot() const { return m_EngineRoot; }

        void SyncRoots();


        std::shared_ptr<Project> m_ActiveProject;
        std::filesystem::path m_EngineRoot;

        static ProjectManager* s_Instance;
    };
}

#endif // CH_PROJECT_MANAGER_H
