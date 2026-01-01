#include "editor/managers/ProjectSceneManager.h"
#include "core/Log.h"
#include <algorithm>
#include <filesystem>
#include <fstream>


ProjectSceneManager::ProjectSceneManager(EditorContext &context) : m_context(context)
{
    LoadRecentScenes();
}

ProjectSceneManager::~ProjectSceneManager()
{
    SaveRecentScenes();
}

void ProjectSceneManager::SetEventCallback(std::function<void(CHEngine::Event &)> callback)
{
    m_eventCallback = callback;
}

void ProjectSceneManager::LoadScene(const std::string &path)
{
    CD_CORE_INFO("ProjectSceneManager: Loading scene %s", path.c_str());
    // Implementation details...
    AddToRecentScenes(path);
}

void ProjectSceneManager::SaveScene(const std::string &path)
{
    std::string savePath = path;
    CD_CORE_INFO("ProjectSceneManager: Saving scene to %s", savePath.c_str());
    // Implementation details...
}

void ProjectSceneManager::NewScene()
{
    CD_CORE_INFO("ProjectSceneManager: Creating new scene");
    // Implementation details...
}

const std::vector<std::string> &ProjectSceneManager::GetRecentScenes() const
{
    return m_recentScenes;
}

bool ProjectSceneManager::HasUnsavedChanges() const
{
    return false; // Placeholder
}

void ProjectSceneManager::AddToRecentScenes(const std::string &path)
{
    auto it = std::find(m_recentScenes.begin(), m_recentScenes.end(), path);
    if (it != m_recentScenes.end())
        m_recentScenes.erase(it);

    m_recentScenes.insert(m_recentScenes.begin(), path);
    if (m_recentScenes.size() > 10)
        m_recentScenes.pop_back();
}

void ProjectSceneManager::LoadRecentScenes()
{
    // Load from disk...
}

void ProjectSceneManager::SaveRecentScenes()
{
    // Save to disk...
}
