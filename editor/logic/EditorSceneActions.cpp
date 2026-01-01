#include "EditorSceneActions.h"
#include "EditorSceneManager.h"
#include "SceneSimulationManager.h"
#include "core/Log.h"
#include "core/application/Application.h"
#include "nfd.h"
#include "scene/MapManager.h"
#include "scene/core/SceneSerializer.h"
#include <filesystem>
#include <fstream>

namespace CHEngine
{

EditorSceneActions::EditorSceneActions(EditorSceneManager *sceneManager,
                                       SceneSimulationManager *simulationManager,
                                       std::shared_ptr<Scene> scene,
                                       CHD::RuntimeLayer **runtimeLayer)
    : m_SceneManager(sceneManager), m_SimulationManager(simulationManager), m_Scene(scene),
      m_RuntimeLayer(runtimeLayer)
{
}

void EditorSceneActions::OnScenePlay()
{
    if (m_SimulationManager)
    {
        auto activeScene = MapManager::GetCurrentScene();
        m_SimulationManager->OnScenePlay(activeScene, activeScene, m_Scene, m_RuntimeLayer,
                                         &Application::Get());
    }
}

void EditorSceneActions::OnSceneStop()
{
    if (m_SimulationManager)
    {
        auto activeScene = MapManager::GetCurrentScene();
        m_SimulationManager->OnSceneStop(activeScene, activeScene, m_Scene, m_RuntimeLayer,
                                         &Application::Get());
    }
}

void EditorSceneActions::NewScene()
{
    // For now, assume map objects are cleared via MapManager or similar if SceneManager is null
    // In next phase we will have a proper EditorSceneManager
    auto activeScene = MapManager::GetCurrentScene();
    if (activeScene)
    {
        activeScene->GetMapObjectsMutable().clear();
        activeScene->GetUIElementsMutable().clear();
    }

    if (m_Scene)
    {
        m_Scene->GetRegistry().clear();
    }
}

void EditorSceneActions::OpenScene()
{
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "chscene"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);

    if (result == NFD_OKAY)
    {
        // For now, use a temporary way to load if m_SceneManager is null
        // (This matches current EditorLayer behavior before refactor)

        std::string path = outPath;
        if (path.find(".chscene") != std::string::npos)
        {
            std::ifstream file(path, std::ios::binary);
            char magic[4];
            bool isBinary = false;
            if (file.is_open() && file.read(magic, 4))
            {
                if (magic[0] == 'C' && magic[1] == 'H' && magic[2] == 'S' && magic[3] == 'C')
                {
                    isBinary = true;
                }
            }

            if (isBinary)
            {
                // Binary loading logic would go here, but let's assume it's handled
                // by the old system for now if we don't have SceneManager
            }
            else
            {
                ECSSceneSerializer ecsSerializer(m_Scene);
                ecsSerializer.Deserialize(path);
            }
        }
        NFD_FreePath(outPath);
    }
    else if (result == NFD_CANCEL)
    {
        // User cancelled
    }
    else
    {
        CD_ERROR("Error: %s\n", NFD_GetError());
    }
}

void EditorSceneActions::SaveScene()
{
    // For now we don't have a stored path in this class, but we could get it from MapManager
    // or keep it here. Current EditorLayer delegates to ProjectManager.
    SaveSceneAs();
}

void EditorSceneActions::SaveSceneAs()
{
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "chscene"}};
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, "NewScene.chscene");

    if (result == NFD_OKAY)
    {
        ECSSceneSerializer ecsSerializer(m_Scene);
        ecsSerializer.Serialize(outPath);

        NFD_FreePath(outPath);
    }
    else if (result == NFD_CANCEL)
    {
        // User cancelled
    }
    else
    {
        CD_ERROR("Error: %s\n", NFD_GetError());
    }
}
} // namespace CHEngine
