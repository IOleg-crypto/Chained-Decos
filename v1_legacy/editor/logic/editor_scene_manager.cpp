#include "editor_scene_manager.h"
#include "core/log.h"
#include "editor/logic/selection_manager.h"
#include "engine/scene/core/scene_serializer.h"

namespace CHEngine
{

EditorSceneManager::EditorSceneManager(SelectionManager *selectionManager)
    : m_SelectionManager(selectionManager), m_activeScene(std::make_shared<Scene>("World Scene")),
      m_uiScene(std::make_shared<Scene>("UI Scene"))
{
}

void EditorSceneManager::ClearScene()
{
    m_activeScene = std::make_shared<Scene>("World Scene");
    m_uiScene = std::make_shared<Scene>("UI Scene");
    m_currentMapPath.clear();
    m_modified = false;
    if (m_SelectionManager)
        m_SelectionManager->ClearSelection();
}

void EditorSceneManager::SaveScene(const std::string &path)
{
    std::string savePath = path.empty() ? m_currentMapPath : path;
    if (savePath.empty())
        return;

    ECSSceneSerializer serializer(m_activeScene);
    serializer.Serialize(savePath);
    m_currentMapPath = savePath;
    m_modified = false;
    CD_CORE_INFO("[EditorSceneManager] Scene saved to: %s", savePath.c_str());
}

void EditorSceneManager::LoadScene(const std::string &path)
{
    auto newScene = std::make_shared<Scene>("Loaded Scene");
    ECSSceneSerializer serializer(newScene);

    if (serializer.Deserialize(path))
    {
        m_activeScene = newScene;
        m_currentMapPath = path;
        m_modified = false;
        if (m_SelectionManager)
            m_SelectionManager->ClearSelection();
        CD_CORE_INFO("[EditorSceneManager] Scene loaded from: %s", path.c_str());
    }
}

} // namespace CHEngine
