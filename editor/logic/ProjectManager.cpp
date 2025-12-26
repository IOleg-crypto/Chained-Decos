#include "ProjectManager.h"
#include "core/Base.h"
#include "scene/resources/map/SceneLoader.h"
#include "scene/resources/map/SceneSerializer.h"
#include <filesystem>
#include <nfd.h>


namespace CHEngine
{
ProjectManager::ProjectManager()
{
    m_ActiveScene = std::make_shared<GameScene>();
}

void ProjectManager::NewScene()
{
    m_ActiveScene = std::make_shared<GameScene>();
    m_ScenePath = "";

    if (m_SceneChangedCallback)
        m_SceneChangedCallback(m_ActiveScene);
}

bool ProjectManager::OpenScene()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[2] = {{"Chained Decos Scene", "chscene"},
                                     {"Legacy JSON Scene", "json"}};
    nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 2, nullptr);

    if (result == NFD_OKAY)
    {
        bool success = OpenScene(outPath);
        NFD_FreePath(outPath);
        return success;
    }
    return false;
}

bool ProjectManager::OpenScene(const std::string &path)
{
    auto newScene = std::make_shared<GameScene>();
    SceneSerializer serializer(newScene);

    if (path.find(".chscene") != std::string::npos)
    {
        if (!serializer.DeserializeBinary(path))
            return false;
    }
    else
    {
        if (!serializer.DeserializeJson(path))
            return false;
    }

    m_ActiveScene = newScene;
    m_ScenePath = path;

    SceneLoader().LoadSkyboxForScene(*m_ActiveScene);

    if (m_SceneChangedCallback)
        m_SceneChangedCallback(m_ActiveScene);

    return true;
}

void ProjectManager::SaveScene()
{
    if (m_ScenePath.empty())
    {
        SaveSceneAs();
    }
    else
    {
        SceneSerializer serializer(m_ActiveScene);
        serializer.SerializeBinary(m_ScenePath);
    }
}

void ProjectManager::SaveSceneAs()
{
    nfdchar_t *outPath = nullptr;
    nfdfilteritem_t filterItem[1] = {{"Chained Decos Scene", "json"}};
    nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, nullptr, "scene.json");

    if (result == NFD_OKAY)
    {
        m_ScenePath = outPath;
        if (m_ScenePath.find(".chscene") == std::string::npos)
            m_ScenePath += ".chscene";

        SceneSerializer serializer(m_ActiveScene);
        serializer.SerializeBinary(m_ScenePath);
        NFD_FreePath(outPath);
    }
}
} // namespace CHEngine
