#ifndef PROJECT_H
#define PROJECT_H

#include <filesystem>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

namespace CHEngine
{

/**
 * @brief Represents a CHEngine project
 *
 * A project contains all game assets, scenes, scripts organized in a directory structure.
 * The project file (.chproject) is a YAML file containing metadata and settings.
 */
class Project
{
public:
    struct ProjectConfig
    {
        std::string name;
        std::string version = "1.0.0";
        std::string engineVersion = "0.1.0";
        std::string startScene;

        struct Directories
        {
            std::string assets = "assets";
            std::string scenes = "scenes";
            std::string scripts = "assets/scripts";
            std::string cache = "cache";
        } directories;

        struct Settings
        {
            struct Physics
            {
                float gravity[3] = {0.0f, -9.81f, 0.0f};
                float fixedTimestep = 0.02f;
            } physics;

            struct Rendering
            {
                bool vsync = true;
                int targetFPS = 60;
            } rendering;

            struct Editor
            {
                int gridSize = 50;
                bool drawWireframe = false;
                bool drawCollisions = false;
            } editor;
        } settings;
    };

public:
    ~Project() = default;

    // --- Static Project Operations ---
public:
    static std::shared_ptr<Project> Create(const std::filesystem::path &directory,
                                           const std::string &name);
    static std::shared_ptr<Project> Load(const std::filesystem::path &projectFilePath);

    // --- Project Operations ---
public:
    void Save();

    // --- Getters & Setters ---
public:
    const std::string &GetName() const;
    const std::filesystem::path &GetProjectDirectory() const;
    const std::filesystem::path &GetProjectFilePath() const;

    std::filesystem::path GetAssetDirectory() const;
    std::filesystem::path GetSceneDirectory() const;
    std::filesystem::path GetScriptDirectory() const;
    std::filesystem::path GetCacheDirectory() const;

    const ProjectConfig &GetConfig() const;
    ProjectConfig &GetConfig();

    // --- Path Helpers ---
public:
    std::filesystem::path GetAbsolutePath(const std::string &relativePath) const;
    std::string GetRelativePath(const std::filesystem::path &absolutePath) const;

    // --- Internal Helpers ---
private:
    Project() = default;

    bool CreateDirectoryStructure();
    bool SerializeToYAML();
    bool DeserializeFromYAML();

    // --- Member Variables ---
private:
    ProjectConfig m_Config;
    std::filesystem::path m_ProjectDirectory;
    std::filesystem::path m_ProjectFilePath;
};

} // namespace CHEngine

#endif // PROJECT_H
