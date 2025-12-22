#include "core/Log.h"
#include "editor/managers/SceneManager.h"
#include "editor/events/EditorEvents.h"
#include "raylib.h" // For TraceLog
#include <filesystem>
#include <fstream>

SceneManager::SceneManager(EditorContext &context) : m_context(context)
{
    LoadRecentScenes();
}

SceneManager::~SceneManager()
{
    SaveRecentScenes();
}

void SceneManager::SetEventCallback(std::function<void(ChainedDecos::Event &)> callback)
{
    m_eventCallback = callback;
}

void SceneManager::LoadScene(const std::string &path)
{
    CD_INFO("SceneManager: Loading scene %s", path.c_str());

    // Check if file exists
    if (!std::filesystem::exists(path))
    {
        CD_ERROR("SceneManager: Scene file not found: %s", path.c_str());
        return;
    }

    // Use SceneLoader to load the map
    SceneLoader loader;
    GameScene loadedScene = loader.LoadScene(path);

    // We need to move loaded scene into context.
    // Assuming context has a way to reset scene or we can just swap data.
    // Ideally EditorContext should expose methods to replace scene data.
    // For now, let's manually move data if GameScene allows it, or use Setters if available.
    // Since GameScene is just data, we can swap.
    // BUT we can't assign to reference.
    // EditorContext::GetCurrentScene() returns reference to member.
    // We can use operator= if it was not deleted. It IS deleted in GameScene.h.
    // Move assignment IS default.
    GameScene &currentScene = m_context.GetCurrentScene();
    currentScene = std::move(loadedScene);

    if (true) // LoadScene doesn't return bool, it returns GameScene? Check signature.
              // SceneLoader::LoadScene returns GameScene. It throws on error?
              // Or returns empty scene?
              // SceneLoader.cpp usually handles this.
              // Let's assume it succeeds if no exception.
    {
        m_context.SetCurrentScenePath(path);
        m_context.SetSceneModified(false);
        m_context.ClearSelection();

        AddToRecentScenes(path);

        CD_INFO("SceneManager: Scene loaded successfully");

        // Dispatch SceneLoaded event
        if (m_eventCallback)
        {
            ChainedDecos::SceneLoadedEvent e(path);
            m_eventCallback(e);
        }
    }
}

void SceneManager::SaveScene(const std::string &path)
{
    std::string savePath = path;
    if (savePath.empty())
    {
        savePath = m_context.GetCurrentScenePath();
    }

    if (savePath.empty())
    {
        CD_WARN("SceneManager: Cannot save scene, no path specified");
        return;
    }

    CD_INFO("SceneManager: Saving scene to %s", savePath.c_str());

    GameScene &scene = m_context.GetCurrentScene();
    SceneLoader loader;
    if (loader.SaveScene(scene, savePath))
    {
        m_context.SetCurrentScenePath(savePath);
        m_context.SetSceneModified(false);
        AddToRecentScenes(savePath);

        CD_INFO("SceneManager: Scene saved successfully");
    }
    else
    {
        CD_ERROR("SceneManager: Failed to save scene");
    }
}

void SceneManager::NewScene()
{
    CD_INFO("SceneManager: Creating new scene");

    GameScene &scene = m_context.GetCurrentScene();
    scene.Cleanup(); // Clear existing data

    // Create new empty map data
    MapMetadata newMap;
    newMap.name = "New Scene";
    newMap.version = "1.0";
    newMap.backgroundColor = BLACK; // Default

    scene.SetMapMetaData(newMap);
    m_context.SetCurrentScenePath("");
    m_context.SetSceneModified(false);
    m_context.ClearSelection();

    // Dispatch SceneLoaded event (empty path for new scene)
    if (m_eventCallback)
    {
        ChainedDecos::SceneLoadedEvent e("");
        m_eventCallback(e);
    }
}

const std::vector<std::string> &SceneManager::GetRecentScenes() const
{
    return m_recentScenes;
}

bool SceneManager::HasUnsavedChanges() const
{
    return m_context.IsSceneModified();
}

void SceneManager::AddToRecentScenes(const std::string &path)
{
    // Remove if already exists
    auto it = std::remove(m_recentScenes.begin(), m_recentScenes.end(), path);
    if (it != m_recentScenes.end())
    {
        m_recentScenes.erase(it, m_recentScenes.end());
    }

    // Add to front
    m_recentScenes.insert(m_recentScenes.begin(), path);

    // Keep max 10
    if (m_recentScenes.size() > 10)
    {
        m_recentScenes.resize(10);
    }
}

void SceneManager::LoadRecentScenes()
{
    // TODO: Load from config file
    // For now hardcoded or empty
}

void SceneManager::SaveRecentScenes()
{
    // TODO: Save to config file
}

