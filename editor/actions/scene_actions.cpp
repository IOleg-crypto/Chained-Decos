#include "scene_actions.h"
#include "editor_layer.h"
#include "engine/core/application.h"
#include "engine/scene/project.h"
#include "engine/scene/scene_serializer.h"
#include "engine/core/dialogs.h"

namespace CHEngine
{
void SceneActions::New()
{
    auto newScene = std::make_shared<Scene>();

    // Ensure every scene starts with a Main Camera
    Entity camera = newScene->CreateEntity("Main Camera");
    auto& cc = camera.AddComponent<CameraComponent>();
    cc.Primary = true;
    camera.GetComponent<TransformComponent>().Translation = {0, 5, 10};

    EditorLayer::Get().SetScene(newScene);
}

void SceneActions::Open()
{
    std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
    auto result = Dialogs::OpenFile(filters);
    if (result)
    {
        Open(*result);
    }
}

void SceneActions::Open(const std::filesystem::path& path)
{
    auto newScene = std::make_shared<Scene>();
    // KISS: Load Game Module BEFORE deserialization to ensure ScriptRegistry is populated
    EditorLayer::Get().SetScene(newScene);
    SceneSerializer serializer(newScene.get());

    if (serializer.Deserialize(path.string()))
    {
        // Sync environment if needed (optional, logic from Application::LoadScene can be moved here or to a helper)
        if (Project::GetActive() && Project::GetActive()->GetEnvironment())
        {
            if (newScene->GetSettings().Environment->GetPath().empty() &&
                newScene->GetSettings().Environment->GetSettings().Skybox.TexturePath.empty())
            {
                newScene->GetSettings().Environment = Project::GetActive()->GetEnvironment();
            }
        }

        // Sync with EditorLayer which manages the scene now
        // Assumes EditorLayer is active (SceneActions is editor-only code)
        newScene->GetSettings().ScenePath = path.string();

        SceneOpenedEvent e(path.string());
        EditorLayer::Get().SetLastScenePath(path.string());
        Application::Get().OnEvent(e);
    }
}

void SceneActions::Save()
{
    auto scene = EditorLayer::Get().GetActiveScene();
    if (scene->GetSettings().ScenePath.empty())
    {
        SaveAs();
        return;
    }

    SceneSerializer serializer(scene.get());
    serializer.Serialize(scene->GetSettings().ScenePath);
    CH_INFO("Scene saved to {0}", scene->GetSettings().ScenePath);
}

void SceneActions::SaveAs()
{
    std::vector<FileDialogFilter> filters = {{"Chained Scene", "chscene"}};
    auto result = Dialogs::SaveFile(filters);
    if (result)
    {
        auto scene = EditorLayer::Get().GetActiveScene();
        scene->GetSettings().ScenePath = result->string();
        SceneSerializer serializer(scene.get());
        serializer.Serialize(result->string());
    }
}

void SceneActions::AutoSave()
{
    auto scene = EditorLayer::Get().GetActiveScene();
    if (!scene || scene->GetSettings().ScenePath.empty())
    {
        return;
    }
 
    SceneSerializer serializer(scene.get());
    serializer.Serialize(scene->GetSettings().ScenePath);
    CH_TRACE("Scene auto-saved to {0}", scene->GetSettings().ScenePath);
}

void SceneActions::SetParent(Entity child, Entity parent)
{
    if (child.HasComponent<HierarchyComponent>())
    {
        child.GetComponent<HierarchyComponent>().Parent = parent;
    }
}
} // namespace CHEngine
