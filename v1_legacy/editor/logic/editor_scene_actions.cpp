#include "editor_scene_actions.h"
#include "core/log.h"
#include "editor_scene_manager.h"
#include "engine/core/application/application.h"
#include "engine/scene/core/scene_manager.h"
#include "nfd.h"
#include "scene_simulation_manager.h"

namespace CHEngine
{
EditorSceneActions::EditorSceneActions()
{
}

void EditorSceneActions::OnScenePlay()
{
    // Need simulation manager access - will refactor this later or use EditorLayer's one for now
    // Actually, let's just use SceneManager's active scene
}

void EditorSceneActions::OnSceneStop()
{
}

void EditorSceneActions::NewScene()
{
    SceneManager::Get().NewScene();
}

void EditorSceneActions::OpenScene()
{
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "chscene"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);

    if (result == NFD_OKAY)
    {
        SceneManager::Get().OpenScene(outPath);
        NFD_FreePath(outPath);
    }
}

void EditorSceneActions::OpenScene(const std::string &path)
{
    SceneManager::Get().OpenScene(path);
}

void EditorSceneActions::SaveScene()
{
    if (SceneManager::Get().GetActiveScenePath().empty())
        SaveSceneAs();
    else
        SceneManager::Get().SaveScene();
}

void EditorSceneActions::SaveSceneAs()
{
    nfdchar_t *outPath = NULL;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "chscene"}};
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, "NewScene.chscene");

    if (result == NFD_OKAY)
    {
        std::string path = outPath;
        if (path.find(".chscene") == std::string::npos)
            path += ".chscene";
        SceneManager::Get().SaveSceneAs(path);
        NFD_FreePath(outPath);
    }
}
} // namespace CHEngine
