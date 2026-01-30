#include "scene_actions.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/scene/scene_serializer.h"
#include "nfd.h"

namespace CHEngine
{
    void SceneActions::New()
    {
        auto newScene = std::make_shared<Scene>();
        Application::Get().SetActiveScene(newScene);
    }

    void SceneActions::Open()
    {
        nfdchar_t *outPath = NULL;
        nfdu8filteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, NULL);
        if (result == NFD_OKAY)
        {
            Open(outPath);
            NFD_FreePath(outPath);
        }
    }

    void SceneActions::Open(const std::filesystem::path &path)
    {
        auto newScene = std::make_shared<Scene>();
        SceneSerializer serializer(newScene.get());
        if (serializer.Deserialize(path.string()))
        {
            Application::Get().SetActiveScene(newScene);
            SceneOpenedEvent e(path.string());
            Application::OnEvent(e);
        }
    }

    void SceneActions::Save()
    {
        auto scene = Application::Get().GetActiveScene();
        SceneSerializer serializer(scene.get());
        // serializer.Serialize(scene->GetPath());
    }

    void SceneActions::SaveAs()
    {
        nfdchar_t *outPath = NULL;
        nfdu8filteritem_t filterItem[1] = {{"Chained Scene", "chscene"}};
        nfdresult_t result = NFD_SaveDialog(&outPath, filterItem, 1, NULL, NULL);
        if (result == NFD_OKAY)
        {
            auto scene = Application::Get().GetActiveScene();
            SceneSerializer serializer(scene.get());
            serializer.Serialize(outPath);
            NFD_FreePath(outPath);
        }
    }

    void SceneActions::SetParent(Entity child, Entity parent)
    {
        if (child.HasComponent<HierarchyComponent>())
        {
            child.GetComponent<HierarchyComponent>().Parent = parent;
        }
    }
} // namespace CHEngine
