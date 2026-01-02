#include "editor_scene_actions.h"
#include "core/application/application.h"
#include "core/log.h"
#include "editor_scene_manager.h"
#include "nfd.h"
#include "scene/core/scene_serializer.h"
#include "scene_simulation_manager.h"
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
        m_SimulationManager->OnScenePlay(m_Scene.get(), m_RuntimeLayer, &Application::Get());
    }
}

void EditorSceneActions::OnSceneStop()
{
    if (m_SimulationManager)
    {
        m_SimulationManager->OnSceneStop(m_Scene.get(), m_RuntimeLayer, &Application::Get());
    }
}

void EditorSceneActions::NewScene()
{
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
        OpenScene(outPath);
        NFD_FreePath(outPath);
    }
}

void EditorSceneActions::OpenScene(const std::string &path)
{
    if (path.find(".chscene") != std::string::npos)
    {
        ECSSceneSerializer ecsSerializer(m_Scene);
        try
        {
            ecsSerializer.Deserialize(path);
        }
        catch (const std::exception &e)
        {
            CD_ERROR("Failed to load scene: %s", e.what());
        }
    }
}

void EditorSceneActions::SaveScene()
{
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
}
} // namespace CHEngine
